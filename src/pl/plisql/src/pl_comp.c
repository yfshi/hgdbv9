/*-------------------------------------------------------------------------
 *
 * pl_comp.c		- Compiler part of the PL/iSQL
 *			  procedural language
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
 *
 *
 * IDENTIFICATION
 *	  src/pl/plisql/src/pl_comp.c
 *
 * add the file for requirement "SQL PARSER"
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include <ctype.h>

#include "miscadmin.h" /* BUG#M0000227 */
#include "access/htup_details.h"
#include "catalog/namespace.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_type.h"
#include "funcapi.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h" /* ReqID:SRS-PLSQL-Collection */
#include "parser/parse_type.h"
#include "plisql.h"
#include "pl_subproc_function.h" /* SRS-PLSQL-SUBPROC */
/* Begin - ReqID:SRS-SQL-PACKAGE */
#include "pl_package.h"
/* End - ReqID:SRS-SQL-PACKAGE */
/* Begin - ReqID:SRS-PLSQL-CURSOR */
#include "pl_udt_type.h"
/* End - ReqID:SRS-PLSQL-CURSOR */
/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
#include "pl_udt_exception.h"
/* End - ReqID:SRS-PLSQL-EXCEPTION */
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/regproc.h"
#include "utils/rel.h"
#include "utils/syscache.h"
#include "utils/typcache.h"
/* Begin - ReqID:SRS-SQL-FUNCOUT */
#include "commands/proclang.h"
/* End - ReqID:SRS-SQL-FUNCOUT */
/* Begin - ReqID:SRS-PLSQL-TYPE */
#include "catalog/pg_package.h"
#include "access/table.h"
#include "catalog/indexing.h"
#include "access/skey.h"
#include "catalog/pg_depend.h"
#include "access/genam.h"
/* End - ReqID:SRS-PLSQL-TYPE */
/* Begin - ReqID:SRS-PLSQL-Collection */
#include "parser/parse_coerce.h"
#include "parser/parse_expr.h"
/* End - ReqID:SRS-PLSQL-Collection */
/* Begin - BUG#M0000376 */
#include "parser/parse_param.h"
/* End - BUG#M0000376 */

#include "parser/pg_object_hook.h"
#include "catalog/pg_trigger.h"
#include "utils/ruleutils.h"
/* Begin - ReqID:SRS-PLSQL-FORALL */
#include "nodes/primnodes.h"
/* Begin - ReqID:SRS-PLSQL-FORALL */

/* ----------
 * Our own local and global variables
 * ----------
 */
PLiSQL_stmt_block *plisql_parse_result;

int	datums_alloc; /* ReqID:SRS-SQL-PACKAGE */
int			plisql_nDatums;
PLiSQL_datum **plisql_Datums;
int	datums_last; /* SRS-PLSQL-SUBPROC */

char	   *plisql_error_funcname;
bool		plisql_DumpExecTree = false;
bool		plisql_check_syntax = false;

PLiSQL_function *plisql_curr_compile;

/* A context appropriate for short-term allocs during compilation */
MemoryContext plisql_compile_tmp_cxt;

/* ----------
 * Hash table for compiled functions
 * ----------
 */
static HTAB *plisql_HashTable = NULL;

#define FUNCS_PER_USER		128 /* initial table size */

/* ----------
 * Lookup table for EXCEPTION condition names
 * ----------
 */
typedef struct
{
	const char *label;
	int			sqlerrstate;
} ExceptionLabelMap;

static const ExceptionLabelMap exception_label_map[] = {
#include "plerrcodes.h"			/* pgrminclude ignore */
	{NULL, 0}
};

/* Begin - ReqID:SRS-PLSQL-TYPE */
bool check_referenced_objects = false;
List *plisql_referenced_objects = NIL;	/*the list elements are ObjectAddress */
/* End - ReqID:SRS-PLSQL-TYPE */

/* Begin - BUG#M0000201: support question marks as binding variables */
int questionmarks_count;
/* End - BUG#M0000201: support question marks as binding variables */

/* ----------
 * static prototypes
 * ----------
 */
static PLiSQL_function *do_compile(FunctionCallInfo fcinfo,
									HeapTuple procTup,
									PLiSQL_function *function,
									PLiSQL_func_hashkey *hashkey,
									bool forValidator);
static Node *plisql_pre_column_ref(ParseState *pstate, ColumnRef *cref);
static Node *plisql_post_column_ref(ParseState *pstate, ColumnRef *cref, Node *var);
static Node *plisql_param_ref(ParseState *pstate, ParamRef *pref);
static Node *resolve_column_ref(ParseState *pstate, PLiSQL_expr *expr,
								ColumnRef *cref, bool error_if_no_field);
static Node *make_datum_param(PLiSQL_expr *expr, int dno, int location);
static PLiSQL_type *build_datatype(HeapTuple typeTup, int32 typmod,
									Oid collation, TypeName *origtypname);
static void compute_function_hashkey(FunctionCallInfo fcinfo,
									 Form_pg_proc procStruct,
									 PLiSQL_func_hashkey *hashkey,
									 bool forValidator,
									 HeapTuple procTup); /* ReqID:SRS-SQL-PACKAGE */
static PLiSQL_function *plisql_HashTableLookup(PLiSQL_func_hashkey *func_key);
static void plisql_HashTableInsert(PLiSQL_function *function,
									PLiSQL_func_hashkey *func_key);
static void plisql_HashTableDelete(PLiSQL_function *function);
/* Begin - ReqID:SRS-PLSQL-TYPE */
static void plisql_add_type_referenced_objects(TypeName *typeName);
/* End - ReqID:SRS-PLSQL-TYPE */
static void add_ora_trigfunc_return(PLiSQL_function *function, TriggerEvent tg_event);
/* Begin - ReqID:SRS-PLSQL-Collection */
static Node *plisql_transform_indirection(ParseState *pstate,
				Node *ind_arg,
				const char *targetName,
				List *indirection,
				Node *rhs,
				int location);
static SubscriptingRef *transformCollectionSubscripts(ParseState *pstate,
							 Node *containerBase,
							 const char *targetName,
							 Oid containerType,
							 Oid elementType,
							 int32 containerTypMod,
							 List *indirection,
							 PLiSQL_type **datatype,
							 Node *rhs,
							 int location);
/* End - ReqID:SRS-PLSQL-Collection */

/* ----------
 * plisql_compile		Make an execution tree for a PL/iSQL function.
 *
 * If forValidator is true, we're only compiling for validation purposes,
 * and so some checks are skipped.
 *
 * Note: it's important for this to fall through quickly if the function
 * has already been compiled.
 * ----------
 */
PLiSQL_function *
plisql_compile(FunctionCallInfo fcinfo, bool forValidator)
{
	Oid			funcOid = fcinfo->flinfo->fn_oid;
	HeapTuple	procTup;
	Form_pg_proc procStruct;
	PLiSQL_function *function;
	PLiSQL_func_hashkey hashkey;
	bool		function_valid = false;
	bool		hashkey_valid = false;
	/* Begin - ReqID:SRS-SQL-PACKAGE */
	Oid rettypeoid = InvalidOid;
	/* End - ReqID:SRS-SQL-PACKAGE */

	/*
	 * Lookup the pg_proc tuple by Oid; we'll need it in any case
	 */
	procTup = SearchSysCache1(PROCOID, ObjectIdGetDatum(funcOid));
	if (!HeapTupleIsValid(procTup))
		elog(ERROR, "cache lookup failed for function %u", funcOid);
	procStruct = (Form_pg_proc) GETSTRUCT(procTup);

	/* Begin - ReqID:SRS-SQL-PACKAGE */
	rettypeoid = get_func_real_rettype(procTup);
	/* End - ReqID:SRS-SQL-PACKAGE */

	/*
	 * See if there's already a cache entry for the current FmgrInfo. If not,
	 * try to find one in the hash table.
	 */
	function = (PLiSQL_function *) fcinfo->flinfo->fn_extra;

recheck:
	if (!function)
	{
		/* Compute hashkey using function signature and actual arg types */
		compute_function_hashkey(fcinfo, procStruct, &hashkey, forValidator,
								procTup); /* ReqID:SRS-SQL-PACKAGE */
		hashkey_valid = true;

		/* And do the lookup */
		function = plisql_HashTableLookup(&hashkey);
	}

	if (function)
	{
		/* We have a compiled function, but is it still valid? */
		if (function->fn_xmin == HeapTupleHeaderGetRawXmin(procTup->t_data) &&
			ItemPointerEquals(&function->fn_tid, &procTup->t_self) &&
			(!OidIsValid(rettypeoid) || rettypeoid == function->fn_orig_rettype)) /* bug#00008129 */
			function_valid = true;
		else
		{
			/*
			 * Nope, so remove it from hashtable and try to drop associated
			 * storage (if not done already).
			 */
			delete_function(function);

			/*
			 * If the function isn't in active use then we can overwrite the
			 * func struct with new data, allowing any other existing fn_extra
			 * pointers to make use of the new definition on their next use.
			 * If it is in use then just leave it alone and make a new one.
			 * (The active invocations will run to completion using the
			 * previous definition, and then the cache entry will just be
			 * leaked; doesn't seem worth adding code to clean it up, given
			 * what a corner case this is.)
			 *
			 * If we found the function struct via fn_extra then it's possible
			 * a replacement has already been made, so go back and recheck the
			 * hashtable.
			 */
			if (function->use_count != 0)
			{
				function = NULL;
				if (!hashkey_valid)
					goto recheck;
			}
		}
	}

	/*
	 * If the function wasn't found or was out-of-date, we have to compile it
	 */
	if (!function_valid)
	{
		/* Begin - ReqID:SRS-PLSQL-TYPE */
		PG_TRY();
		{
			if (!forValidator &&
				procStruct->prostatus == PROSTATUS_INVALID)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("%s %s is in an invalid state",
								(procStruct->prokind == PROKIND_FUNCTION) ? "function" : "procedure",
								NameStr(procStruct->proname))));
		/* End - ReqID:SRS-PLSQL-TYPE */

			/*
			 * Calculate hashkey if we didn't already; we'll need it to store the
			 * completed function.
			 */
			if (!hashkey_valid)
				compute_function_hashkey(fcinfo, procStruct, &hashkey,
										 forValidator, procTup); /* ReqID:SRS-SQL-PACKAGE */
	
			/*
			 * Do the hard part.
			 */
			function = do_compile(fcinfo, procTup, function,
								  &hashkey, forValidator);
		/* Begin - ReqID:SRS-PLSQL-TYPE */
		}
		PG_CATCH();
		{
			ReleaseSysCache(procTup);
			PG_RE_THROW();
		}
		PG_END_TRY();
		/* End - ReqID:SRS-PLSQL-TYPE */
	}

	ReleaseSysCache(procTup);

	/*
	 * Save pointer in FmgrInfo to avoid search on subsequent calls
	 */
	fcinfo->flinfo->fn_extra = (void *) function;

	/*
	 * Finally return the compiled function
	 */
	return function;
}

/*
 * This is the slow part of plisql_compile().
 *
 * The passed-in "function" pointer is either NULL or an already-allocated
 * function struct to overwrite.
 *
 * While compiling a function, the CurrentMemoryContext is the
 * per-function memory context of the function we are compiling. That
 * means a palloc() will allocate storage with the same lifetime as
 * the function itself.
 *
 * Because palloc()'d storage will not be immediately freed, temporary
 * allocations should either be performed in a short-lived memory
 * context or explicitly pfree'd. Since not all backend functions are
 * careful about pfree'ing their allocations, it is also wise to
 * switch into a short-term context before calling into the
 * backend. An appropriate context for performing short-term
 * allocations is the plisql_compile_tmp_cxt.
 *
 * NB: this code is not re-entrant.  We assume that nothing we do here could
 * result in the invocation of another plisql function.
 */
static PLiSQL_function *
do_compile(FunctionCallInfo fcinfo,
		   HeapTuple procTup,
		   PLiSQL_function *function,
		   PLiSQL_func_hashkey *hashkey,
		   bool forValidator)
{
	Form_pg_proc procStruct = (Form_pg_proc) GETSTRUCT(procTup);
	bool		is_dml_trigger = CALLED_AS_TRIGGER(fcinfo);
	bool		is_event_trigger = CALLED_AS_EVENT_TRIGGER(fcinfo);
	Datum		prosrcdatum;
	bool		isnull;
	char	   *proc_source;
	HeapTuple	typeTup;
	Form_pg_type typeStruct;
	PLiSQL_variable *var;
	PLiSQL_rec *rec;
	int			i;
	ErrorContextCallback plerrcontext;
	int			parse_rc;
	Oid			rettypeid;
	int			numargs;
	int			num_in_args = 0;
	int			num_out_args = 0;
	Oid		   *argtypes;
	char	  **argnames;
	char	   *argmodes;
	int		   *in_arg_varnos = NULL;
	PLiSQL_variable **out_arg_variables;
	MemoryContext func_cxt;

	/* Begin - ReqID:SRS-SQL-FUNCOUT */
	bool		is_plisql_function = false;
	int			*all_arg_varnos = NULL;
	/* End - ReqID:SRS-SQL-FUNCOUT */

	/* Begin - ReqID:SRS-SQL-PACKAGE */
	char		**argtypenames = NULL;
	char		*rettypename = NULL;
	PLiSQL_type	*rettype = NULL;
	/* End - ReqID:SRS-SQL-PACKAGE */
	/* Begin - BUG#M0000227 */
	Oid 		define_useid = InvalidOid;
	Oid 		save_userid;
	int 		save_sec_context;
	Oid			current_user = GetUserId();
	/* End - BUG#M0000227 */
	/* Begin - ReqID:SRS-PLSQL-RECORD */
	bool		has_udt_type = false;
	/* End - ReqID:SRS-PLSQL-RECORD */

	/* Begin - ReqID:SRS-SQL-TRIGGER */
	PgObjectInfo 	objinfo = NULL;
	ObjectAddress 	address;
	Oid				funcOid = fcinfo->flinfo->fn_oid;
	Oid				trigoid = InvalidOid;
	bool			is_ora_trig = false;
	/* End - ReqID:SRS-SQL-TRIGGER */

	/*
	 * Setup the scanner input and error info.  We assume that this function
	 * cannot be invoked recursively, so there's no need to save and restore
	 * the static variables used here.
	 */
	prosrcdatum = SysCacheGetAttr(PROCOID, procTup,
								  Anum_pg_proc_prosrc, &isnull);
	if (isnull)
		elog(ERROR, "null prosrc");
	proc_source = TextDatumGetCString(prosrcdatum);
	plisql_scanner_init(proc_source);

	plisql_error_funcname = pstrdup(NameStr(procStruct->proname));

	/*
	 * Setup error traceback support for ereport()
	 */
	plerrcontext.callback = plisql_compile_error_callback;
	plerrcontext.arg = forValidator ? proc_source : NULL;
	plerrcontext.previous = error_context_stack;
	error_context_stack = &plerrcontext;

	/* Begin - BUG#M0000227 */
	/*
	 * invoke as define in compiling, we should
	 * switch current user for plsql
	 */
	if (compile_plisql_use_owner &&
		current_user != procStruct->proowner)
	{
		define_useid = procStruct->proowner;
		GetUserIdAndSecContext(&save_userid, &save_sec_context);

		SetUserIdAndSecContext(define_useid,
								save_sec_context | SECURITY_LOCAL_USERID_CHANGE);
	}
	/* End - BUG#M0000227 */

	/*
	 * Do extra syntax checks when validating the function definition. We skip
	 * this when actually compiling functions for execution, for performance
	 * reasons.
	 */
	plisql_check_syntax = forValidator;

	/*
	 * Create the new function struct, if not done already.  The function
	 * structs are never thrown away, so keep them in TopMemoryContext.
	 */
	if (function == NULL)
	{
		function = (PLiSQL_function *)
			MemoryContextAllocZero(TopMemoryContext, sizeof(PLiSQL_function));
	}
	else
	{
		/* re-using a previously existing struct, so clear it out */
		memset(function, 0, sizeof(PLiSQL_function));
	}
	plisql_curr_compile = function;

	/*
	 * All the permanent output of compilation (e.g. parse tree) is kept in a
	 * per-function memory context, so it can be reclaimed easily.
	 */
	func_cxt = AllocSetContextCreate(TopMemoryContext,
									 "PL/iSQL function",
									 ALLOCSET_DEFAULT_SIZES);
	plisql_compile_tmp_cxt = MemoryContextSwitchTo(func_cxt);

	function->fn_signature = format_procedure(fcinfo->flinfo->fn_oid);
	MemoryContextSetIdentifier(func_cxt, function->fn_signature);
	function->fn_oid = fcinfo->flinfo->fn_oid;
	function->fn_xmin = HeapTupleHeaderGetRawXmin(procTup->t_data);
	function->fn_tid = procTup->t_self;
	function->fn_input_collation = fcinfo->fncollation;
	function->fn_cxt = func_cxt;
	function->out_param_varno = -1; /* set up for no OUT param */
	function->resolve_option = plisql_variable_conflict;
	function->print_strict_params = plisql_print_strict_params;
	/* only promote extra warnings and errors at CREATE FUNCTION time */
	function->extra_warnings = forValidator ? plisql_extra_warnings : 0;
	function->extra_errors = forValidator ? plisql_extra_errors : 0;
	/* Begin - ReqID:SRS-SQL-LIBPQOUT */
	function->paramnames = NULL;
	/* End - ReqID:SRS-SQL-LIBPQOUT */

	if (is_dml_trigger)
		function->fn_is_trigger = PLISQL_DML_TRIGGER;
	else if (is_event_trigger)
		function->fn_is_trigger = PLISQL_EVENT_TRIGGER;
	else
		function->fn_is_trigger = PLISQL_NOT_TRIGGER;

	function->fn_prokind = procStruct->prokind;

	function->nstatements = 0;
	function->requires_procedure_resowner = false;
	function->fn_intisnumber = procStruct->prointisnumber;	/* BUG:M0000068 */

	/* Begin - ReqID:SRS-SQL-FUNCOUT */
	function->fn_ret_vardno = -1;
	function->fn_no_return = false;

	if (function->fn_prokind == PROKIND_FUNCTION &&
		LANG_PLISQL_OID == procStruct->prolang)
		is_plisql_function = true;
	/* End - ReqID:SRS-SQL-FUNCOUT */

	/* Begin - ReqID:SRS-SQL-PACKAGE */
	function->namelabel = pstrdup(NameStr(procStruct->proname));
	/* End - ReqID:SRS-SQL-PACKAGE */

	/* Begin - ReqID:SRS-PLSQL-DSQL */
	function->paramCheck = NULL;
	function->oraCallParamCheck = -1;
	/* End - ReqID:SRS-PLSQL-DSQL */

	/* Begin - ReqID:SRS-PLSQL-TYPE */
	check_referenced_objects = true;
	plisql_referenced_objects = NIL;
	/* End - ReqID:SRS-PLSQL-TYPE */

	/* Begin - BUG#M0000201: support question marks as binding variables */
	questionmarks_count = 0;
	/* End - BUG#M0000201: support question marks as binding variables */

	/*
	 * Initialize the compiler, particularly the namespace stack.  The
	 * outermost namespace contains function parameters and other special
	 * variables (such as FOUND), and is named after the function itself.
	 */
	plisql_ns_init();
	plisql_ns_push(NameStr(procStruct->proname), PLISQL_LABEL_BLOCK);
	/* Begin - ReqID:SRS-PLSQL-GOTO */
	plisql_start_label_compile(NameStr(procStruct->proname));
	/* End - ReqID:SRS-PLSQL-GOTO */
	plisql_DumpExecTree = false;
	cur_compile_func_level = 0;
	plisql_start_datums();
	/* Begin - SRS-PLSQL-SUBPROC */
	plisql_start_subproc_func();
	/* End - SRS-PLSQL-SUBPROC */
	/* Begin - ReqID:SRS-SQL-PACKAGE */
	plisql_compile_packageitem = NULL;
	plisql_saved_compile[cur_compile_func_level] = function;
	Assert(plisql_curr_global_proper_level == 0);
	plisql_IdentifierLookup = IDENTIFIER_LOOKUP_DECLARE;
	/* End - ReqID:SRS-SQL-PACKAGE */

	switch (function->fn_is_trigger)
	{
		case PLISQL_NOT_TRIGGER:

			/*
			 * Fetch info about the procedure's parameters. Allocations aren't
			 * needed permanently, so make them in tmp cxt.
			 *
			 * We also need to resolve any polymorphic input or output
			 * argument types.  In validation mode we won't be able to, so we
			 * arbitrarily assume we are dealing with integers.
			 */
			MemoryContextSwitchTo(plisql_compile_tmp_cxt);

			numargs = get_func_arg_info(procTup,
										&argtypes, &argnames, &argmodes);

			/* Begin - ReqID:SRS-SQL-PACKAGE */
			get_func_typename_info(procTup,
									&argtypenames, &rettypename);
			/* End - ReqID:SRS-SQL-PACKAGE */

			plisql_resolve_polymorphic_argtypes(numargs, argtypes, argmodes,
												 fcinfo->flinfo->fn_expr,
												 forValidator,
												 plisql_error_funcname);

			in_arg_varnos = (int *) palloc(numargs * sizeof(int));
			/* Begin - ReqID:SRS-SQL-FUNCOUT */
			if (is_plisql_function)
				out_arg_variables = (PLiSQL_variable **) palloc((numargs + 1) * sizeof(PLiSQL_variable *));
			else
			/* End - ReqID:SRS-SQL-FUNCOUT */
				out_arg_variables = (PLiSQL_variable **) palloc(numargs * sizeof(PLiSQL_variable *));

			/* Begin - ReqID:SRS-SQL-FUNCOUT */
			all_arg_varnos = (int *) palloc(numargs * sizeof(int));
			/* End - ReqID:SRS-SQL-FUNCOUT */

			MemoryContextSwitchTo(func_cxt);

			/*
			 * Create the variables for the procedure's parameters.
			 */
			for (i = 0; i < numargs; i++)
			{
				char		buf[32];
				Oid			argtypeid = argtypes[i];
				char		argmode = argmodes ? argmodes[i] : PROARGMODE_IN;
				PLiSQL_type *argdtype;
				PLiSQL_variable *argvariable;
				PLiSQL_nsitem_type argitemtype;

				/* Create $n name for variable */
				snprintf(buf, sizeof(buf), "$%d", i + 1);

				/* Create datatype info */
				/* Begin - ReqID:SRS-SQL-PACKAGE */
				/* consider the argument type comes from a package */
				if (argtypenames != NULL && strcmp(argtypenames[i], "") != 0)
				{
					TypeName	*tname;

					tname = (TypeName *) stringToNode(argtypenames[i]);

					if (tname->pct_type || tname->row_type/* ReqID:SRS-PLSQL-TYPE */)
						argdtype = plisql_parse_package_type(tname, parse_by_var_type, false);
					else
						argdtype = plisql_parse_package_type(tname, parse_by_pkg_type, false);
					if (argdtype == NULL)
					{
						/* Begin - ReqID:SRS-PLSQL-TYPE */
						/*
						 * Check if parameter type refers to %TYPE or %ROWTYPE
						 */
						Type		typtup;

						typtup = LookupOraTypeName(NULL, tname, NULL, true);
						if (typtup == NULL)
							ereport(ERROR,
									(errcode(ERRCODE_UNDEFINED_OBJECT),
									 errmsg("type \"%s\" does not exist",
											TypeNameToString(tname)),
									 parser_errposition(NULL, tname->location)));

						argtypeid = typeTypeId(typtup);
						argdtype = plisql_build_datatype(argtypeid,
														  -1,
														  function->fn_input_collation,
														  NULL);
						ReleaseSysCache(typtup);
						/* End - ReqID:SRS-PLSQL-TYPE */
					}
					pfree(tname);
				}
				/* End - ReqID:SRS-SQL-PACKAGE */
				else
				{
					/* Begin - BUG:M0000068 */
					/*
					 * All intger and smallint datatypes in function args list
					 * will be convert to number when the function compiling if
					 * prointisnumber is true.
					 */
					if (procStruct->prointisnumber &&
						(argtypeid == INT2OID || argtypeid == INT4OID))
						argdtype = plisql_build_datatype(NUMBEROID,
														  NUMBER38TYPMOD,
														  function->fn_input_collation,
														  NULL);
					else
					/* End - BUG:M0000068 */
						argdtype = plisql_build_datatype(argtypeid,
														  -1,
														  function->fn_input_collation,
														  NULL);
				}

				/* Disallow pseudotype argument */
				/* (note we already replaced polymorphic types) */
				/* (build_variable would do this, but wrong message) */
				if (argdtype->ttype == PLISQL_TTYPE_PSEUDO)
					ereport(ERROR,
							(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							 errmsg("PL/iSQL functions cannot accept type %s",
									format_type_be(argtypeid))));

				/*
				 * Build variable and add to datum list.  If there's a name
				 * for the argument, use that as refname, else use $n name.
				 */
				argvariable = plisql_build_variable((argnames &&
													  argnames[i][0] != '\0') ?
													 argnames[i] : buf,
													 0, argdtype, false);

				if (argvariable->dtype == PLISQL_DTYPE_VAR)
				{
					argitemtype = PLISQL_NSTYPE_VAR;

					/* Begin - ReqID:SRS-SQL-FUNCOUT */
					/* 
					 * Use this info field to distinguish the variable' IN, OUT, IN OUT mode, 
					 * IN mode variable is not allowned to assign value.
					 */
					if (argmode == PROARGMODE_IN ||
						argmode == PROARGMODE_OUT ||
						argmode == PROARGMODE_INOUT)
						((PLiSQL_var *)argvariable)->info = argmode;
					else
						((PLiSQL_var *)argvariable)->info = PROARGMODE_IN;
					/* End - ReqID:SRS-SQL-FUNCOUT */
				}
				else
				{
					Assert(argvariable->dtype == PLISQL_DTYPE_REC ||
						argvariable->dtype == PLISQL_DTYPE_ORA_RECORD/* SRS-PLSQL-RECORD */);

					if (argvariable->dtype == PLISQL_DTYPE_REC)
					{
						argitemtype = PLISQL_NSTYPE_REC;

						/* Begin - ReqID:SRS-SQL-FUNCOUT */
						/*
						 * Use this info field to distinguish the variable' IN, OUT, IN OUT mode,
						 * IN mode variable is not allowned to assign value.
						 */
						if (argmode == PROARGMODE_IN ||
							argmode == PROARGMODE_OUT ||
							argmode == PROARGMODE_INOUT)
							((PLiSQL_rec *)argvariable)->info = argmode;
						else
							((PLiSQL_rec *)argvariable)->info = PROARGMODE_IN;
						/* End - ReqID:SRS-SQL-FUNCOUT */
					}
					/* Begin - ReqID:SRS-PLSQL-RECORD */
					else
					{
						argitemtype = PLISQL_NSTYPE_ORA_RECORD;

						/*
						 * Use this info field to distinguish the variable' IN, OUT, IN OUT mode,
						 * IN mode variable is not allowned to assign value.
						 */
						if (argmode == PROARGMODE_IN ||
							argmode == PROARGMODE_OUT ||
							argmode == PROARGMODE_INOUT)
							((PLiSQL_row *)argvariable)->info = argmode;
						else
							((PLiSQL_row *)argvariable)->info = PROARGMODE_IN;

						if (argmode == PROARGMODE_OUT ||
							argmode == PROARGMODE_INOUT)
							has_udt_type = true;
					}
					/* End - ReqID:SRS-PLSQL-RECORD */

				}

				/* Remember arguments in appropriate arrays */
				if (argmode == PROARGMODE_IN ||
					argmode == PROARGMODE_INOUT ||
					argmode == PROARGMODE_VARIADIC)
					in_arg_varnos[num_in_args++] = argvariable->dno;
				if (argmode == PROARGMODE_OUT ||
					argmode == PROARGMODE_INOUT ||
					argmode == PROARGMODE_TABLE)
					out_arg_variables[num_out_args++] = argvariable;

				/* Begin - ReqID:SRS-SQL-FUNCOUT */
				all_arg_varnos[i] = argvariable->dno;
				/* End - ReqID:SRS-SQL-FUNCOUT */

				/* Add to namespace under the $n name */
				add_parameter_name(argitemtype, argvariable->dno, buf);
				/* Begin - ReqID:SRS-PLSQL-CURSOR */
				if (ISORACURSOROID(argdtype->typoid))
				{
					set_plisql_cursorvar_nsitem_typoid(argvariable, true);
				}
				/* End - ReqID:SRS-PLSQL-CURSOR */

				/* If there's a name for the argument, make an alias */
				if (argnames && argnames[i][0] != '\0')
				{
					add_parameter_name(argitemtype, argvariable->dno,
									   argnames[i]);
					/* Begin - ReqID:SRS-PLSQL-CURSOR */
					if (ISORACURSOROID(argdtype->typoid))
					{
						set_plisql_cursorvar_nsitem_typoid(argvariable, false);
					}
					/* End - ReqID:SRS-PLSQL-CURSOR */
				}
			}

			/* Begin - ReqID:SRS-SQL-PACKAGE */
			if (rettypename != NULL)
			{
				TypeName		*tname;

				tname = (TypeName *) stringToNode(rettypename);
				if (tname->pct_type || tname->row_type/* ReqID:SRS-PLSQL-TYPE */)
					rettype = plisql_parse_package_type(tname, parse_by_var_type, false);
				else
					rettype = plisql_parse_package_type(tname, parse_by_pkg_type, false);

				if (rettype == NULL)
				{
					/* Begin - ReqID:SRS-PLSQL-TYPE */
					/*
					 * Check if return type refers to %TYPE or %ROWTYPE
					 */
					Type		typtup;

					typtup = LookupOraTypeName(NULL, tname, NULL, true);
					if (typtup == NULL)
						ereport(ERROR,
								(errcode(ERRCODE_UNDEFINED_OBJECT),
								 errmsg("type \"%s\" does not exist",
										TypeNameToString(tname)),
								 parser_errposition(NULL, tname->location)));

					rettypeid = typeTypeId(typtup);
					rettype = plisql_build_datatype(rettypeid,
													  -1,
													  function->fn_input_collation,
													  NULL);
					ReleaseSysCache(typtup);
					/* End - ReqID:SRS-PLSQL-TYPE */

				}
				else
					rettypeid = rettype->typoid;

				/* Begin - ReqID:SRS-PLSQL-RECORD */
				function->rettype = rettype;
				/* End - ReqID:SRS-PLSQL-RECORD */

				pfree(tname);
			}
			/* End - ReqID:SRS-SQL-PACKAGE */
			else
				rettypeid = procStruct->prorettype;

			/* Begin - bug#00008129 */
			function->fn_orig_rettype = rettypeid;
			/* End - bug#00008129 */

			/* Begin - ReqID:SRS-SQL-FUNCOUT */
			/*Build a '_retval_' varaible for the function return value */
			if (num_out_args > 0 && is_plisql_function)
			{
				if (procStruct->prorettype != VOIDOID)
				{
					char		buf[32];
					PLiSQL_type *argdtype;
					PLiSQL_variable *argvariable;
					PLiSQL_nsitem_type argitemtype;
					Oid 		argtypeid = rettypeid; /* ReqID:SRS-SQL-PACKAGE */

					/* Is the function return type a PSEUDO type? */
					if (IsPolymorphicType(argtypeid))
					{
						if (forValidator)
						{
							if (argtypeid == ANYARRAYOID)
								argtypeid = INT4ARRAYOID;
							else if (argtypeid == ANYRANGEOID)
								argtypeid = INT4RANGEOID;
							else
								argtypeid = INT4OID;
						}
						else
						{
							argtypeid = get_fn_expr_rettype(fcinfo->flinfo);
							if (!OidIsValid(argtypeid))
								ereport(ERROR,
										(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
											errmsg("could not determine actual return type "
													"for polymorphic function \"%s\"",
														plisql_error_funcname)));
						}
					}

					snprintf(buf, sizeof(buf), "$%d", i + 1);

					/* Begin - BUG:M0000068 */
					/*
					 * The function return type will be convert to number when the function compiling if
					 * prointisnumber is true.
					 */
					/* Begin - ReqID:SRS-SQL-PACKAGE */
					if (rettype != NULL)
						argdtype = rettype;
					/* End - ReqID:SRS-SQL-PACKAGE */
					else if (procStruct->prointisnumber &&
						(argtypeid == INT2OID || argtypeid == INT4OID))
						argdtype = plisql_build_datatype(NUMBEROID,
														  NUMBER38TYPMOD,
														  function->fn_input_collation,
														  NULL);
					else
					/* End - BUG:M0000068 */
						argdtype = plisql_build_datatype(argtypeid,
															 -1,
															 function->fn_input_collation,
															 NULL);
					argvariable = plisql_build_variable("_RETVAL_",
															 0, argdtype, false);
					function->fn_ret_vardno = argvariable->dno;

					if (argvariable->dtype == PLISQL_DTYPE_VAR)
						argitemtype = PLISQL_NSTYPE_VAR;
					else
						argitemtype = PLISQL_NSTYPE_REC;

					if (result_first)
					{
						int		j;

						/* the new builded variable for function return value is the first of out_arg_variables array. */
						for (j = num_out_args; j > 0; j--)
							out_arg_variables[j] = out_arg_variables[j - 1];
						out_arg_variables[0] = argvariable;
					}
					else
					{
						/* the new builded variable for function return value is after all function OUT args */
						out_arg_variables[num_out_args] = argvariable;
					}
					num_out_args++;

					add_parameter_name(argitemtype, argvariable->dno, buf);
					add_parameter_name(argitemtype, argvariable->dno, "_RETVAL_");
				}
			}
			/* End - ReqID:SRS-SQL-FUNCOUT */

			/*
			 * If there's at least one OUT parameter in a procedure, build a row that
			 * holds all of them.  If there's at least one OUT parameter in a function,
			 * and the return type is VOID,build a row that holds all of OUT parameter.
			 * If the function return type is not VOID, build a row that combined all OUT parameters and return value.
			 * Procedures and function return a row even for one OUT parameter.
			 */
			if (num_out_args > 0)
			{
				PLiSQL_row *row = build_row_from_vars(out_arg_variables,
													   num_out_args);

				plisql_adddatum((PLiSQL_datum *) row);
				function->out_param_varno = row->dno;
			}

			/*
			 * Check for a polymorphic returntype. If found, use the actual
			 * returntype type from the caller's FuncExpr node, if we have
			 * one.  (In validation mode we arbitrarily assume we are dealing
			 * with integers.)
			 *
			 * Note: errcode is FEATURE_NOT_SUPPORTED because it should always
			 * work; if it doesn't we're in some context that fails to make
			 * the info available.
			 */

			/* Begin - ReqID:SRS-SQL-FUNCOUT */
			/* If a function have out arguments, we change function return type to RECORDOID */
			if (num_out_args > 0 &&
				rettypeid != RECORDOID &&
				is_plisql_function)
				rettypeid = RECORDOID;
			/* End - ReqID:SRS-SQL-FUNCOUT */

			if (IsPolymorphicType(rettypeid))
			{
				if (forValidator)
				{
					if (rettypeid == ANYARRAYOID ||
						rettypeid == ANYCOMPATIBLEARRAYOID)
						rettypeid = INT4ARRAYOID;
					else if (rettypeid == ANYRANGEOID ||
							 rettypeid == ANYCOMPATIBLERANGEOID)
						rettypeid = INT4RANGEOID;
					else if (rettypeid == ANYMULTIRANGEOID)
						rettypeid = INT4MULTIRANGEOID;
					else		/* ANYELEMENT or ANYNONARRAY or ANYCOMPATIBLE */
						rettypeid = INT4OID;
					/* XXX what could we use for ANYENUM? */
				}
				else
				{
					rettypeid = get_fn_expr_rettype(fcinfo->flinfo);
					if (!OidIsValid(rettypeid))
						ereport(ERROR,
								(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
								 errmsg("could not determine actual return type "
										"for polymorphic function \"%s\"",
										plisql_error_funcname)));
				}
			}

			/* Begin - BUG:M0000068 */
			if (procStruct->prointisnumber &&
				(rettypeid == INT2OID || rettypeid == INT4OID))
				rettypeid = NUMBEROID;
			/* End - BUG:M0000068 */
			
			/*
			 * Normal function has a defined returntype
			 */
			function->fn_rettype = rettypeid;
			function->fn_retset = procStruct->proretset;

			/*
			 * Lookup the function's return type
			 */
			typeTup = SearchSysCache1(TYPEOID, ObjectIdGetDatum(rettypeid));
			if (!HeapTupleIsValid(typeTup))
				elog(ERROR, "cache lookup failed for type %u", rettypeid);
			typeStruct = (Form_pg_type) GETSTRUCT(typeTup);

			/* Disallow pseudotype result, except VOID or RECORD */
			/* (note we already replaced polymorphic types) */
			if (typeStruct->typtype == TYPTYPE_PSEUDO)
			{
				if (rettypeid == VOIDOID ||
					rettypeid == RECORDOID ||
					/* Begin - ReqID:SRS-PLSQL-RECORD */
					rettypeid == ORARECORDOID)
					/* End - ReqID:SRS-PLSQL-RECORD */
					 /* okay */ ;
				else if (rettypeid == TRIGGEROID || rettypeid == EVENT_TRIGGEROID)
					ereport(ERROR,
							(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							 errmsg("trigger functions can only be called as triggers")));
				else
					ereport(ERROR,
							(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							 errmsg("PL/iSQL functions cannot return type %s",
									format_type_be(rettypeid))));
			}

			function->fn_retistuple = type_is_rowtype(rettypeid);
			function->fn_retisdomain = (typeStruct->typtype == TYPTYPE_DOMAIN);
			function->fn_retbyval = typeStruct->typbyval;
			function->fn_rettyplen = typeStruct->typlen;
			/* Begin - ReqID:SRS-PLSQL-RECORD */
			function->fn_hasudtype = has_udt_type;
			/* End - ReqID:SRS-PLSQL-RECORD */

			/*
			 * install $0 reference, but only for polymorphic return types,
			 * and not when the return is specified through an output
			 * parameter.
			 */
			if (IsPolymorphicType(procStruct->prorettype) &&
				num_out_args == 0)
			{
				(void) plisql_build_variable("$0", 0,
											  build_datatype(typeTup,
															 -1,
															 function->fn_input_collation,
															 NULL),
											  true);
			}

			ReleaseSysCache(typeTup);
			break;

		case PLISQL_DML_TRIGGER:
			/* Trigger procedure's return type is unknown yet */
			function->fn_rettype = InvalidOid;
			function->fn_retbyval = false;
			function->fn_retistuple = true;
			function->fn_retisdomain = false;
			function->fn_retset = false;

			/* shouldn't be any declared arguments */
			if (procStruct->pronargs != 0)
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_FUNCTION_DEFINITION),
						 errmsg("trigger functions cannot have declared arguments"),
						 errhint("The arguments of the trigger can be accessed through TG_NARGS and TG_ARGV instead.")));

			/* Begin - ReqID:SRS-SQL-TRIGGER */
			/*
			 * Get the dependency relationship and obtain the trigger
			 * Oid that the function depends on
			 */
			trigoid = checkAutoDepOnOraTrigAndTrigFunc(funcOid);
			if (OidIsValid(trigoid))
			{
				ObjectAddressSet(address, TriggerRelationId, trigoid);
				objinfo =  pg_object_get_ori_infor_hook(address);
			}

			if (objinfo && objinfo->oricompatible_mode == 'o')
			{
				char		*new_alias = NULL;
				char		*old_alias = NULL;

				if (objinfo->orioption)
				{
					text		*key;
					JsonbValue	*v;
					JsonbValue	vbuf;
					key = cstring_to_text("referencing");

					v = getKeyJsonValueFromContainer(&objinfo->orioption->root,
													 VARDATA_ANY(key),
													 VARSIZE_ANY_EXHDR(key),
													 &vbuf);

					if (v != NULL && v->type == jbvBinary)
					{
						char	*str = NULL;
						str = JsonbToCString(NULL, v->val.binary.data, v->val.binary.len);
						parse_referencing_string(str, &old_alias, &new_alias);
					}
				}

				/* using the referencing NEW alias build the record */
				rec = plisql_build_record("new", 0, NULL, RECORDOID, true);
				if (new_alias)
					add_parameter_name(PLISQL_NSTYPE_REC, rec->dno, psprintf(":%s", new_alias));
				else
					add_parameter_name(PLISQL_NSTYPE_REC, rec->dno, ":new");
				function->new_varno = rec->dno;

				/* using the referencing OLD alias build the record */
				rec = plisql_build_record("old", 0, NULL, RECORDOID, true);
				if (old_alias)
					add_parameter_name(PLISQL_NSTYPE_REC, rec->dno, psprintf(":%s", old_alias));
				else
					add_parameter_name(PLISQL_NSTYPE_REC, rec->dno, ":old");
				function->old_varno = rec->dno;

				is_ora_trig = true;		/* BUG#Z7179 */
			}
			else
			{
				/* Add the record for referencing NEW ROW */
				rec = plisql_build_record("new", 0, NULL, RECORDOID, true);
				add_parameter_name(PLISQL_NSTYPE_REC, rec->dno, ":new");	/* BUG#Z6196 */
				function->new_varno = rec->dno;

				/* Add the record for referencing OLD ROW */
				rec = plisql_build_record("old", 0, NULL, RECORDOID, true);
				add_parameter_name(PLISQL_NSTYPE_REC, rec->dno, ":old");	/* BUG#Z6196 */
				function->old_varno = rec->dno;
			}
			/* End - ReqID:SRS-SQL-TRIGGER */

			/* Add the variable tg_name */
			var = plisql_build_variable("tg_name", 0,
										 plisql_build_datatype(NAMEOID,
																-1,
																function->fn_input_collation,
																NULL),
										 true);
			Assert(var->dtype == PLISQL_DTYPE_VAR);
			var->dtype = PLISQL_DTYPE_PROMISE;
			((PLiSQL_var *) var)->promise = PLISQL_PROMISE_TG_NAME;

			/* Add the variable tg_when */
			var = plisql_build_variable("tg_when", 0,
										 plisql_build_datatype(TEXTOID,
																-1,
																function->fn_input_collation,
																NULL),
										 true);
			Assert(var->dtype == PLISQL_DTYPE_VAR);
			var->dtype = PLISQL_DTYPE_PROMISE;
			((PLiSQL_var *) var)->promise = PLISQL_PROMISE_TG_WHEN;

			/* Add the variable tg_level */
			var = plisql_build_variable("tg_level", 0,
										 plisql_build_datatype(TEXTOID,
																-1,
																function->fn_input_collation,
																NULL),
										 true);
			Assert(var->dtype == PLISQL_DTYPE_VAR);
			var->dtype = PLISQL_DTYPE_PROMISE;
			((PLiSQL_var *) var)->promise = PLISQL_PROMISE_TG_LEVEL;

			/* Add the variable tg_op */
			var = plisql_build_variable("tg_op", 0,
										 plisql_build_datatype(TEXTOID,
																-1,
																function->fn_input_collation,
																NULL),
										 true);
			Assert(var->dtype == PLISQL_DTYPE_VAR);
			var->dtype = PLISQL_DTYPE_PROMISE;
			((PLiSQL_var *) var)->promise = PLISQL_PROMISE_TG_OP;

			/* Add the variable tg_relid */
			var = plisql_build_variable("tg_relid", 0,
										 plisql_build_datatype(OIDOID,
																-1,
																InvalidOid,
																NULL),
										 true);
			Assert(var->dtype == PLISQL_DTYPE_VAR);
			var->dtype = PLISQL_DTYPE_PROMISE;
			((PLiSQL_var *) var)->promise = PLISQL_PROMISE_TG_RELID;

			/* Add the variable tg_relname */
			var = plisql_build_variable("tg_relname", 0,
										 plisql_build_datatype(NAMEOID,
																-1,
																function->fn_input_collation,
																NULL),
										 true);
			Assert(var->dtype == PLISQL_DTYPE_VAR);
			var->dtype = PLISQL_DTYPE_PROMISE;
			((PLiSQL_var *) var)->promise = PLISQL_PROMISE_TG_TABLE_NAME;

			/* tg_table_name is now preferred to tg_relname */
			var = plisql_build_variable("tg_table_name", 0,
										 plisql_build_datatype(NAMEOID,
																-1,
																function->fn_input_collation,
																NULL),
										 true);
			Assert(var->dtype == PLISQL_DTYPE_VAR);
			var->dtype = PLISQL_DTYPE_PROMISE;
			((PLiSQL_var *) var)->promise = PLISQL_PROMISE_TG_TABLE_NAME;

			/* add the variable tg_table_schema */
			var = plisql_build_variable("tg_table_schema", 0,
										 plisql_build_datatype(NAMEOID,
																-1,
																function->fn_input_collation,
																NULL),
										 true);
			Assert(var->dtype == PLISQL_DTYPE_VAR);
			var->dtype = PLISQL_DTYPE_PROMISE;
			((PLiSQL_var *) var)->promise = PLISQL_PROMISE_TG_TABLE_SCHEMA;

			/* Add the variable tg_nargs */
			var = plisql_build_variable("tg_nargs", 0,
										 plisql_build_datatype(INT4OID,
																-1,
																InvalidOid,
																NULL),
										 true);
			Assert(var->dtype == PLISQL_DTYPE_VAR);
			var->dtype = PLISQL_DTYPE_PROMISE;
			((PLiSQL_var *) var)->promise = PLISQL_PROMISE_TG_NARGS;

			/* Add the variable tg_argv */
			var = plisql_build_variable("tg_argv", 0,
										 plisql_build_datatype(TEXTARRAYOID,
																-1,
																function->fn_input_collation,
																NULL),
										 true);
			Assert(var->dtype == PLISQL_DTYPE_VAR);
			var->dtype = PLISQL_DTYPE_PROMISE;
			((PLiSQL_var *) var)->promise = PLISQL_PROMISE_TG_ARGV;

			break;

		case PLISQL_EVENT_TRIGGER:
			function->fn_rettype = VOIDOID;
			function->fn_retbyval = false;
			function->fn_retistuple = true;
			function->fn_retisdomain = false;
			function->fn_retset = false;

			/* shouldn't be any declared arguments */
			if (procStruct->pronargs != 0)
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_FUNCTION_DEFINITION),
						 errmsg("event trigger functions cannot have declared arguments")));

			/* Add the variable tg_event */
			var = plisql_build_variable("tg_event", 0,
										 plisql_build_datatype(TEXTOID,
																-1,
																function->fn_input_collation,
																NULL),
										 true);
			Assert(var->dtype == PLISQL_DTYPE_VAR);
			var->dtype = PLISQL_DTYPE_PROMISE;
			((PLiSQL_var *) var)->promise = PLISQL_PROMISE_TG_EVENT;

			/* Add the variable tg_tag */
			var = plisql_build_variable("tg_tag", 0,
										 plisql_build_datatype(TEXTOID,
																-1,
																function->fn_input_collation,
																NULL),
										 true);
			Assert(var->dtype == PLISQL_DTYPE_VAR);
			var->dtype = PLISQL_DTYPE_PROMISE;
			((PLiSQL_var *) var)->promise = PLISQL_PROMISE_TG_TAG;

			break;

		default:
			elog(ERROR, "unrecognized function typecode: %d",
				 (int) function->fn_is_trigger);
			break;
	}

	/* Remember if function is STABLE/IMMUTABLE */
	function->fn_readonly = (procStruct->provolatile != PROVOLATILE_VOLATILE);

	/*
	 * Create the magic FOUND variable.
	 */
	var = plisql_build_variable("found", 0,
								 plisql_build_datatype(BOOLOID,
														-1,
														InvalidOid,
														NULL),
								 true);
	function->found_varno = var->dno;

	/* Begin - ReqID:SRS-PLSQL-CURSOR */
	/*
	 * build sql variable
	 */
	var = plisql_build_variable("sql", 0,
								plisql_build_datatype(SYS_REFCURSOROID,
													-1,
													InvalidOid,
													NULL),
								true);
	function->sql_varno = var->dno;
	/* End - ReqID:SRS-PLSQL-CURSOR */

	/* Begin - ReaID: OA-2124 */
	var = plisql_build_variable("$$plsql_unit", 0,
								plisql_build_datatype(ORAVARCHARCHAROID,
														-1,
														InvalidOid,
														NULL),
								true);
	function->plsql_unit_varno = var->dno;

	var = plisql_build_variable("$$plsql_unit_type", 0,
								plisql_build_datatype(ORAVARCHARCHAROID,
														-1,
														InvalidOid,
														NULL),
								true);
	function->plsql_unit_type_varno = var->dno;

	var = plisql_build_variable("$$plsql_line", 0,
								plisql_build_datatype(INT4OID,
														-1,
														InvalidOid,
														NULL),
								true);
	function->plsql_line_varno = var->dno;
	/* End - ReaID: OA-2124 */

	/* Begin - ReqID:SRS-PLSQL-TYPE */
	PG_TRY();
	{
	/* End - ReqID:SRS-PLSQL-TYPE */
		/*
		 * Now parse the function's text
		 */
		parse_rc = plisql_yyparse();
		if (parse_rc != 0)
			elog(ERROR, "plisql parser returned %d", parse_rc);
	/* Begin - ReqID:SRS-PLSQL-TYPE */
	}
	PG_CATCH();
	{
		list_free(plisql_referenced_objects);
		/* Begin - BUG#Z00006811 */
		if (OidIsValid(define_useid))
			SetUserIdAndSecContext(save_userid, save_sec_context);
		/* End - BUG#Z00006811 */
		PG_RE_THROW();
	}
	PG_END_TRY();
	/* End - ReqID:SRS-PLSQL-TYPE */

	/* Begin - ReqID:SRS-PLSQL-GOTO */
	plisql_finish_label_compile();
	/* End - ReqID:SRS-PLSQL-GOTO */

	function->action = plisql_parse_result;

	/* Begin - ReqID:SRS-SQL-FUNCOUT */
	/* Begin - BUG#Z195: function body is NULL */
	if (function->action->body == NIL &&
		/* Begin - BUG#Z209 */
		is_plisql_function)
		/* End - BUG#Z209 */
		function->fn_no_return = true;
	/* End - BUG#Z195: function body is NULL */
	else if (num_out_args > 0 &&
		is_plisql_function &&
		procStruct->prorettype != VOIDOID &&
		((PLiSQL_stmt *) llast(function->action->body))->cmd_type != PLISQL_STMT_RETURN)
		function->fn_no_return = true;
	/* End - ReqID:SRS-SQL-FUNCOUT */

	/*
	 * If it has OUT parameters or returns VOID or returns a set, we allow
	 * control to fall off the end without an explicit RETURN statement. The
	 * easiest way to implement this is to add a RETURN statement to the end
	 * of the statement list during parsing.
	 */
	if (num_out_args > 0 || function->fn_rettype == VOIDOID ||
		function->fn_retset)
		add_dummy_return(function);

	/* Begin - ReqID:SRS-SQL-TRIGGER */
	/*
	 * If ORA trigger function, here we add the RETURN clause
	 */
	if (is_ora_trig)
	{
		TriggerData *trigdata = (TriggerData *) fcinfo->context;	/* BUG#Z7179 */
		add_ora_trigfunc_return(function, trigdata->tg_event); 		/* BUG#Z6170 */
	}
	/* End - ReqID:SRS-SQL-TRIGGER */

	/*
	 * Complete the function's info
	 */
	function->fn_nargs = procStruct->pronargs;

	/* Begin - BUG#Z0001268 */
	/*
	 * palloc space for fn_argvarnos
	 */
	if (function->fn_nargs > 0)
		function->fn_argvarnos = (int *) palloc0(sizeof(int) * function->fn_nargs);
	/* End - BUG#Z00001268 */

	/* Begin - ReqID:SRS-SQL-FUNCOUT */
	if (is_plisql_function)
	{
		for (i = 0; i < function->fn_nargs; i++)
			function->fn_argvarnos[i] = all_arg_varnos[i];
	}
	else
	/* End - ReqID:SRS-SQL-FUNCOUT */
		for (i = 0; i < function->fn_nargs; i++)
			/* Begin - BUG#Z196: procedure fn_argvarnos include out arguments */
			function->fn_argvarnos[i] = all_arg_varnos[i];
			/* End - BUG#Z196: procedure with out arguments supports overload */

	plisql_finish_datums(function);

	/* Begin - SRS-PLSQL-SUBPROC */
	plisql_finish_subproc_func(function);
	/* End - SRS-PLSQL-SUBPROC */

	/* Begin - ReqID:SRS-SQL-PACKAGE */
	plisql_check_subproc_define(function);

	/*
	 * after plisql_check_subproc_define for nice error message
	 */
	plisql_scanner_finish();
	pfree(proc_source);
	/* End - ReqID:SRS-SQL-PACKAGE */

	/* Debug dump for completed functions */
	if (plisql_DumpExecTree)
		plisql_dumptree(function, 0, 0); /* ReqID:SRS-SQL-PACKAGE */

	/* Begin - ReqID:SRS-PLSQL-TYPE */
	if (check_referenced_objects)
	{
		/*
		 * Have builded a global list plisql_referenced_objects
		 * to save the objects referenced by %TYPE and %ROWTYPE
		 * in function body. Then consider whether there are
		 * referenced objects in the function arguments datatype
		 * and the function returned datatype.
		 *
		 */
		char		**argtypenames = NULL;
		char		*rettypename = NULL;

		get_func_typename_info(procTup, &argtypenames, &rettypename);
		if (argtypenames != NULL)
		{
			int 	i = 0;

			for (i = 0; i < procStruct->pronargs; i++)
			{
				if (strcmp(argtypenames[i], "") != 0)
				{
					TypeName	*typeName;

					typeName = (TypeName *) stringToNode(argtypenames[i]);
					plisql_add_type_referenced_objects(typeName);
				}
			}
			pfree(argtypenames);
		}
		if (rettypename != NULL && strcmp(rettypename, "") != 0)
		{
			TypeName	*typeName;

			typeName = (TypeName *) stringToNode(rettypename);
			plisql_add_type_referenced_objects(typeName);
			pfree(rettypename);
		}
	
		/* All the referenced objects are in plisql_referenced_objects list */
		if (plisql_referenced_objects != NIL)
		{
			ListCell   *x;
			ObjectAddresses *addrs;
			ObjectAddress myself;
			Relation	depRel;

			/*
			 * If the parameters or return type of a function(procedure)
			 * or the function body reference tablename.column%TYPE,
			 * schemaname.tablename.column%TYPE, tablename%ROWTYPE,
			 * schemaname.tablename%ROWTYPE, record the dependencies
			 * between relation and function(procedure or package).
			 */
			addrs = new_object_addresses();
			ObjectAddressSet(myself, ProcedureRelationId, function->fn_oid);
			depRel = table_open(DependRelationId, RowExclusiveLock);

			/* Scan the parameters list and check if it references table name or tablename.columnname */
			foreach(x, plisql_referenced_objects)
			{
				ObjectAddress *obj = (ObjectAddress *) lfirst(x);
				ObjectAddress referenced;
				ScanKeyData key[3];
				int 		nkeys;
				SysScanDesc scan;
				HeapTuple	tup;
				bool		isduplicate = false;

				/* Check duplicate dependencies that alreay been builded in CREATE FUNCTION/PROCEDURE statment */
				ScanKeyInit(&key[0],
							Anum_pg_depend_refclassid,
							BTEqualStrategyNumber, F_OIDEQ,
							ObjectIdGetDatum(obj->classId));
				ScanKeyInit(&key[1],
							Anum_pg_depend_refobjid,
							BTEqualStrategyNumber, F_OIDEQ,
							ObjectIdGetDatum(obj->objectId));
				if (obj->objectSubId != 0)
				{
					/* Consider only dependencies of this sub-object */
					ScanKeyInit(&key[2],
								Anum_pg_depend_refobjsubid,
								BTEqualStrategyNumber, F_INT4EQ,
								Int32GetDatum(obj->objectSubId));
					nkeys = 3;
				}
				else
				{
					/* Consider dependencies of this object and any sub-objects it has */
					nkeys = 2;
				}
				scan = systable_beginscan(depRel, DependReferenceIndexId, true,
										  NULL, nkeys, key);
				while (HeapTupleIsValid(tup = systable_getnext(scan)))
				{
					Form_pg_depend foundDep = (Form_pg_depend) GETSTRUCT(tup);
					if (foundDep->objid == function->fn_oid)
					{
						isduplicate = true;
						break;
					}
				}
				systable_endscan(scan);
				if (isduplicate)
					continue;
				/*
				 * Record the dependencies between relation and function(procedure, or package).
				 */
				ObjectAddressSet(referenced, obj->classId, obj->objectId);
				referenced.objectSubId = obj->objectSubId;
				add_exact_object_address(&referenced, addrs);
			}
			table_close(depRel, RowExclusiveLock);
			record_object_address_dependencies(&myself, addrs, DEPENDENCY_TYPE);

			free_object_addresses(addrs);
		}
	}
	/* End - ReqID:SRS-PLSQL-TYPE */

	/* Begin - BUG#M0000227 */
	if (OidIsValid(define_useid))
		SetUserIdAndSecContext(save_userid, save_sec_context);
	/* End - BUG#M0000227 */

	/*
	 * add it to the hash table
	 */
	plisql_HashTableInsert(function, hashkey);

	/*
	 * Pop the error context stack
	 */
	error_context_stack = plerrcontext.previous;
	plisql_error_funcname = NULL;

	plisql_check_syntax = false;

	/* Begin - ReqID:SRS-PLSQL-TYPE */
	check_referenced_objects = false;
	list_free(plisql_referenced_objects);
	/* End - ReqID:SRS-PLSQL-TYPE */

	/* Begin - BUG#M0000201: support question marks as binding variables */
	questionmarks_count = 0;
	/* End - BUG#M0000201: support question marks as binding variables */

	MemoryContextSwitchTo(plisql_compile_tmp_cxt);
	plisql_compile_tmp_cxt = NULL;
	return function;
}

/* ----------
 * plisql_compile_inline	Make an execution tree for an anonymous code block.
 *
 * Note: this is generally parallel to do_compile(); is it worth trying to
 * merge the two?
 *
 * Note: we assume the block will be thrown away so there is no need to build
 * persistent data structures.
 * ----------
 */
PLiSQL_function *
plisql_compile_inline(char *proc_source, ParamListInfo inparams/* ReqID:SRS-SQL-ANONYOUT */, bool fromcall /* ReqID:SRS-SQL-CALL */)
{
	char	   *func_name = "inline_code_block";
	PLiSQL_function *function;
	ErrorContextCallback plerrcontext;
	PLiSQL_variable *var;
	int			parse_rc;
	MemoryContext func_cxt;

	/*
	 * Setup the scanner input and error info.  We assume that this function
	 * cannot be invoked recursively, so there's no need to save and restore
	 * the static variables used here.
	 */
	plisql_scanner_init(proc_source);

	plisql_error_funcname = func_name;

	/*
	 * Setup error traceback support for ereport()
	 */
	plerrcontext.callback = plisql_compile_error_callback;
	plerrcontext.arg = proc_source;
	plerrcontext.previous = error_context_stack;
	error_context_stack = &plerrcontext;

	/* Do extra syntax checking if check_function_bodies is on */
	plisql_check_syntax = check_function_bodies;

	/* Function struct does not live past current statement */
	function = (PLiSQL_function *) palloc0(sizeof(PLiSQL_function));

	plisql_curr_compile = function;

	/*
	 * All the rest of the compile-time storage (e.g. parse tree) is kept in
	 * its own memory context, so it can be reclaimed easily.
	 */
	func_cxt = AllocSetContextCreate(CurrentMemoryContext,
									 "PL/iSQL inline code context",
									 ALLOCSET_DEFAULT_SIZES);
	plisql_compile_tmp_cxt = MemoryContextSwitchTo(func_cxt);

	function->fn_signature = pstrdup(func_name);
	function->fn_is_trigger = PLISQL_NOT_TRIGGER;
	function->fn_input_collation = InvalidOid;
	function->fn_cxt = func_cxt;
	function->out_param_varno = -1; /* set up for no OUT param */
	function->resolve_option = plisql_variable_conflict;
	function->print_strict_params = plisql_print_strict_params;

	/*
	 * don't do extra validation for inline code as we don't want to add spam
	 * at runtime
	 */
	function->extra_warnings = 0;
	function->extra_errors = 0;

	function->nstatements = 0;
	function->requires_procedure_resowner = false;

	/* Begin - ReqID:SRS-SQL-FUNCOUT */
	function->fn_ret_vardno = -1;
	function->fn_no_return = false;
	/* End - ReqID:SRS-SQL-FUNCOUT */

	/* Begin - ReqID:SRS-SQL-PACKAGE */
	function->namelabel = NULL;
	/* End - ReqID:SRS-SQL-PACKAGE */
	function->do_from_call = fromcall; /* ReqID:SRS-SQL-CALL */

	/* Begin - ReqID:SRS-PLSQL-TYPE */
	check_referenced_objects = false;
	plisql_referenced_objects = NIL;
	/* End - ReqID:SRS-PLSQL-TYPE */

	/* Begin - BUG#M0000201: support question marks as binding variables */
	questionmarks_count = 0;
	/* End - BUG#M0000201: support question marks as binding variables */

	/* Begin - ReqID:SRS-PLSQL-GOTO */
	plisql_start_label_compile(func_name);
	/* End - ReqID:SRS-PLSQL-GOTO */
	plisql_ns_init();
	plisql_ns_push(func_name, PLISQL_LABEL_BLOCK);
	plisql_DumpExecTree = false;
	cur_compile_func_level = 0;
	plisql_start_datums();
	/* Begin - SRS-PLSQL-SUBPROC */
	plisql_start_subproc_func();
	/* End - SRS-PLSQL-SUBPROC */
	/* Begin - ReqID:SRS-SQL-PACKAGE */
	plisql_compile_packageitem = NULL;
	Assert(plisql_curr_global_proper_level == 0);
	plisql_saved_compile[cur_compile_func_level] = function;
	/* End - ReqID:SRS-SQL-PACKAGE */

	/* Set up as though in a function returning VOID */
	function->fn_rettype = VOIDOID;
	function->fn_retset = false;
	function->fn_retistuple = false;
	function->fn_retisdomain = false;
	function->fn_prokind = PROKIND_ANONYMOUS_BLOCK;	/* anonymous block */
	/* a bit of hardwired knowledge about type VOID here */
	function->fn_retbyval = true;
	function->fn_rettyplen = sizeof(int32);
	/* Begin - ReqID:SRS-PLSQL-RECORD */
	function->fn_hasudtype = false;
	/* End - ReqID:SRS-PLSQL-RECORD */
	/* Begin - ReqID:SRS-PLSQL-DSQL */
	function->paramCheck = NULL;
	function->oraCallParamCheck = -1;
	/* End - ReqID:SRS-PLSQL-DSQL */

	/*
	 * Remember if function is STABLE/IMMUTABLE.  XXX would it be better to
	 * set this true inside a read-only transaction?  Not clear.
	 */
	function->fn_readonly = false;

	/* Begin - ReqID:SRS-SQL-ANONYOUT */
	/*
	 * Create the variables for the procedure's parameters.
	 */
	function->fn_nargs = 0;
	if (inparams)
	{
		int		i;

		function->fn_nargs = inparams->numParams;
		/* Begin - ReqID:SRS-SQL-LIBPQOUT */
		function->paramnames = inparams->paramnames;
		/* End - ReqID:SRS-SQL-LIBPQOUT */
		/* Begin - ReqID:SRS-PLSQL-DSQL */
		function->paramCheck = inparams->paramCheck;
		function->oraCallParamCheck = inparams->oraCallParamCheck;
		/* End - ReqID:SRS-PLSQL-DSQL */

		/* Begin - BUG#Z0001268 */
		/*
		 * The parameters of the anonymous block may exceed the maximum
		 * value of function parameters FUNC_MAX_ARGS, and increasing FUNC_MAX_ARGS
		 * will waste memory space From mobile applications, the number of parameters
		 * in anonymous blocks reaches 50,000. This obviously means that a function
		 * needs to waste 4 million bytes, which is not reasonable.
		 * so they are saved with extra pointers
		 */
		if (function->fn_nargs > 0)
		{
			function->fn_argvarnos = (int *) palloc0(sizeof(int) * function->fn_nargs);
		}
		/* End - BUG#Z00001268 */

		for (i = 0; i < inparams->numParams; i++)
		{
			ParamExternData *param = &(inparams->params[i]);
			char		buf[32];
			Oid			argtypeid = param->ptype;
			PLiSQL_type *argdtype;
			PLiSQL_variable *argvariable;
			int			argitemtype;

			/* Create $n name for variable */
			/* Begin - ReqID:SRS-SQL-LIBPQOUT */
			if (inparams->paramnames != NULL &&
				inparams->paramnames[i] != NULL)
				snprintf(buf, sizeof(buf), "%s", inparams->paramnames[i]);
			else
			/* End - ReqID:SRS-SQL-LIBPQOUT */
				snprintf(buf, sizeof(buf), "$%d", i + 1);

			/* Begin - ReqID:SRS-PLSQL-DSQL */
			if (param->pdatatype)
			{
				/* 
				 * For the ora record type, when we building a new datatype 
				 * by typeid, we can't set its record arglist. So, we reference 
				 * its datatype to original one for the new variable. 
				 */
				argdtype = param->pdatatype;
			}
			else
			/* End - ReqID:SRS-PLSQL-DSQL */
			{
				/* Create datatype info */
				argdtype = plisql_build_datatype(argtypeid,
											param->ptypmod, /* ReqID:SRS-CMD-PSQL */
											function->fn_input_collation,
											NULL);
			}

			/* Disallow pseudotype argument */
			/* (note we already replaced polymorphic types) */
			/* (build_variable would do this, but wrong message) */
			if (argdtype->ttype != PLISQL_TTYPE_SCALAR &&
				argdtype->ttype != PLISQL_TTYPE_REC &&
				argdtype->ttype != PLISQL_TTYPE_UDT_REF_CURSOR && /* ReqID:SRS-PLSQL-CURSOR */
				argdtype->ttype != PLISQL_TTYPE_SUBTYPE && /* ReqID:SRS-PLSQL-SUBTYPE */
				argdtype->ttype != PLISQL_TTYPE_ORARECORD_TYPE && /* ReqID:SRS-PLSQL-DSQL */
				argdtype->ttype != PLISQL_TTYPE_COLLECT_TYPE) /* ReqID:SRS-PLSQL-Collection */
				ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg("PLI/SQL functions cannot accept type %s",
				format_type_be(argtypeid))));

			/* Build variable and add to datum list */
			argvariable = plisql_build_variable(buf, 0,
												argdtype, false);

			if (argvariable->dtype == PLISQL_DTYPE_VAR)
			{
				argitemtype = PLISQL_NSTYPE_VAR;
				/* Begin - ReqID:SRS-SQL-ANONYOUT */
				((PLiSQL_var *)argvariable)->info = param->pmode;
				/* End - ReqID:SRS-SQL-ANONYOUT */
			}
			/* Begin - ReqID:SRS-PLSQL-DSQL */
			else if (argvariable->dtype == PLISQL_DTYPE_ORA_RECORD)
			{
				argitemtype = PLISQL_NSTYPE_ORA_RECORD;
				((PLiSQL_row *)argvariable)->info = param->pmode;
			}
			/* End - ReqID:SRS-PLSQL-DSQL */
			else
			{
				Assert(argvariable->dtype == PLISQL_DTYPE_REC);
				argitemtype = PLISQL_NSTYPE_REC;
				/* Begin - ReqID:SRS-SQL-ANONYOUT */
				((PLiSQL_rec *)argvariable)->info = param->pmode;
				/* End - ReqID:SRS-SQL-ANONYOUT */
			}

			/* Add to namespace under the $n name */
			add_parameter_name(argitemtype, argvariable->dno, buf);
			/* Begin - ReqID:SRS-PLSQL-CURSOR */
			if (ISORACURSOROID(argdtype->typoid))
			{
				set_plisql_cursorvar_nsitem_typoid(argvariable, true);
			}
			/* End - ReqID:SRS-PLSQL-CURSOR */
			function->fn_argvarnos[i] = argvariable->dno;

			/* Begin - ReqID:SRS-PLSQL-DSQL */
			if (!function->fn_hasudtype)
			{
				if (argvariable->dtype == PLISQL_DTYPE_ORA_RECORD)
					function->fn_hasudtype = true;
			}
			/* End - ReqID:SRS-PLSQL-DSQL */
		}
	}
	/* End - ReqID:SRS-SQL-ANONYOUT */

	/*
	 * Create the magic FOUND variable.
	 */
	var = plisql_build_variable("found", 0,
								 plisql_build_datatype(BOOLOID,
														-1,
														InvalidOid,
														NULL),
								 true);
	function->found_varno = var->dno;

	/* Begin - ReqID:SRS-PLSQL-CURSOR */
	/*
	 * build sql variable
	 */
	var = plisql_build_variable("sql", 0,
								plisql_build_datatype(SYS_REFCURSOROID,
													-1,
													InvalidOid,
													NULL),
								true);
	function->sql_varno = var->dno;
	/* End - ReqID:SRS-PLSQL-CURSOR */

	/* Begin - ReaID: OA-2124 */
	var = plisql_build_variable("$$plsql_unit", 0,
								plisql_build_datatype(ORAVARCHARCHAROID,
														-1,
														InvalidOid,
														NULL),
								true);
	function->plsql_unit_varno = var->dno;

	var = plisql_build_variable("$$plsql_unit_type", 0,
								plisql_build_datatype(ORAVARCHARCHAROID,
														-1,
														InvalidOid,
														NULL),
								true);
	function->plsql_unit_type_varno = var->dno;

	var = plisql_build_variable("$$plsql_line", 0,
								plisql_build_datatype(INT4OID,
														-1,
														InvalidOid,
														NULL),
								true);
	function->plsql_line_varno = var->dno;
	/* End - ReaID: OA-2124 */

	/*
	 * Now parse the function's text
	 */
	parse_rc = plisql_yyparse();
	if (parse_rc != 0)
		elog(ERROR, "plisql parser returned %d", parse_rc);
	function->action = plisql_parse_result;

	/* Begin - ReqID:SRS-PLSQL-GOTO */
	plisql_finish_label_compile();
	/* End - ReqID:SRS-PLSQL-GOTO */

	/*
	 * If it returns VOID (always true at the moment), we allow control to
	 * fall off the end without an explicit RETURN statement.
	 */
	if (function->fn_rettype == VOIDOID)
		add_dummy_return(function);

	/*
	 * Complete the function's info
	 */
	plisql_finish_datums(function);

	/* Begin - SRS-PLSQL-SUBPROC */
	plisql_finish_subproc_func(function);
	/* End - SRS-PLSQL-SUBPROC */

	/* Begin - ReqID:SRS-SQL-PACKAGE */
	plisql_check_subproc_define(function);

	/*
	 * after plisql_check_subproc_define for nice error message
	 */
	plisql_scanner_finish();
	/* End - ReqID:SRS-SQL-PACKAGE */

	/*
	 * Pop the error context stack
	 */
	error_context_stack = plerrcontext.previous;
	plisql_error_funcname = NULL;

	plisql_check_syntax = false;

	/* Begin - ReqID:SRS-PLSQL-TYPE */
	check_referenced_objects = false;
	plisql_referenced_objects = NIL;
	/* End - ReqID:SRS-PLSQL-TYPE */

	/* Begin - BUG#M0000201: support question marks as binding variables */
	questionmarks_count = 0;
	/* End - BUG#M0000201: support question marks as binding variables */

	MemoryContextSwitchTo(plisql_compile_tmp_cxt);
	plisql_compile_tmp_cxt = NULL;
	return function;
}


/*
 * error context callback to let us supply a call-stack traceback.
 * If we are validating or executing an anonymous code block, the function
 * source text is passed as an argument.
 */
void
plisql_compile_error_callback(void *arg)
{
	if (arg)
	{
		/*
		 * Try to convert syntax error position to reference text of original
		 * CREATE FUNCTION or DO command.
		 */
		if (function_parse_error_transpose((const char *) arg))
			return;

		/*
		 * Done if a syntax error position was reported; otherwise we have to
		 * fall back to a "near line N" report.
		 */
	}

	if (plisql_error_funcname)
		errcontext("compilation of PL/iSQL function \"%s\" near line %d",
				   plisql_error_funcname, plisql_latest_lineno());
}


/*
 * Add a name for a function parameter to the function's namespace
 */
void
add_parameter_name(PLiSQL_nsitem_type itemtype, int itemno, const char *name)
{
	/*
	 * Before adding the name, check for duplicates.  We need this even though
	 * functioncmds.c has a similar check, because that code explicitly
	 * doesn't complain about conflicting IN and OUT parameter names.  In
	 * plisql, such names are in the same namespace, so there is no way to
	 * disambiguate.
	 */
	if (plisql_ns_lookup(plisql_ns_top(), true,
						  name, NULL, NULL,
						  NULL) != NULL)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_FUNCTION_DEFINITION),
				 errmsg("parameter name \"%s\" used more than once",
						name)));

	/* OK, add the name */
	plisql_ns_additem(itemtype, itemno, name);
}

/*
 * Add a dummy RETURN statement to the given function's body
 */
void
add_dummy_return(PLiSQL_function *function)
{
	/*
	 * If the outer block has an EXCEPTION clause, we need to make a new outer
	 * block, since the added RETURN shouldn't act like it is inside the
	 * EXCEPTION clause.  Likewise, if it has a label, wrap it in a new outer
	 * block so that EXIT doesn't skip the RETURN.
	 */
	if (function->action->exceptions != NULL ||
		function->action->label != NULL)
	{
		PLiSQL_stmt_block *new;

		new = palloc0(sizeof(PLiSQL_stmt_block));
		new->cmd_type = PLISQL_STMT_BLOCK;
		new->stmtid = ++function->nstatements;
		new->body = list_make1(function->action);

		function->action = new;
	}
	if (function->action->body == NIL ||
		/* Begin - ReqID:SRS-SQL-FUNCOUT */
		function->fn_ret_vardno != -1 ||
		(function->fn_ret_vardno == -1 &&
		/* End - ReqID:SRS-SQL-FUNCOUT */
		((PLiSQL_stmt *) llast(function->action->body))->cmd_type != PLISQL_STMT_RETURN))
	{
		PLiSQL_stmt_return *new;

		new = palloc0(sizeof(PLiSQL_stmt_return));
		new->cmd_type = PLISQL_STMT_RETURN;
		new->stmtid = ++function->nstatements;
		new->expr = NULL;
		new->retvarno = function->out_param_varno;

		function->action->body = lappend(function->action->body, new);
	}
}

static void
add_ora_trigfunc_return(PLiSQL_function *function, TriggerEvent tg_event)
{
	/*
	 * If the outer block has an EXCEPTION clause, we need to make a new outer
	 * block, since the added RETURN shouldn't act like it is inside the
	 * EXCEPTION clause.  Likewise, if it has a label, wrap it in a new outer
	 * block so that EXIT doesn't skip the RETURN.
	 */
	if (function->action->exceptions != NULL ||
		function->action->label != NULL)
	{
		PLiSQL_stmt_block *new;

		new = palloc0(sizeof(PLiSQL_stmt_block));
		new->cmd_type = PLISQL_STMT_BLOCK;
		new->stmtid = ++function->nstatements;
		new->body = list_make1(function->action);

		function->action = new;
	}
	if (function->action->body == NIL ||
		/* Begin - ReqID:SRS-SQL-FUNCOUT */
		function->fn_ret_vardno != -1 ||
		(function->fn_ret_vardno == -1 &&
		/* End - ReqID:SRS-SQL-FUNCOUT */
		((PLiSQL_stmt *) llast(function->action->body))->cmd_type != PLISQL_STMT_RETURN))
	{
		PLiSQL_stmt_return *new;

		new = palloc0(sizeof(PLiSQL_stmt_return));
		new->cmd_type = PLISQL_STMT_RETURN;
		new->stmtid = ++function->nstatements;
		new->expr = NULL;
		if (TRIGGER_FIRED_FOR_ROW(tg_event) &&		/* INSERT and UPDATE */
			(TRIGGER_FIRED_BY_INSERT(tg_event) || TRIGGER_FIRED_BY_UPDATE(tg_event)))
			new->retvarno = function->new_varno;
		else if (TRIGGER_FIRED_FOR_ROW(tg_event) &&	/* DELETE */
				 TRIGGER_FIRED_BY_DELETE(tg_event))
			new->retvarno = function->old_varno;
		else
			new->retvarno = function->out_param_varno;

		function->action->body = lappend(function->action->body, new);
	}
}

/*
 * plisql_parser_setup		set up parser hooks for dynamic parameters
 *
 * Note: this routine, and the hook functions it prepares for, are logically
 * part of plisql parsing.  But they actually run during function execution,
 * when we are ready to evaluate a SQL query or expression that has not
 * previously been parsed and planned.
 */
void
plisql_parser_setup(struct ParseState *pstate, PLiSQL_expr *expr)
{
	pstate->p_pre_columnref_hook = plisql_pre_column_ref;
	pstate->p_post_columnref_hook = plisql_post_column_ref;
	pstate->p_paramref_hook = plisql_param_ref;
	/* Begin - SRS-PLSQL-SUBPROC */
	pstate->p_subprocfunc_hook = plisql_subprocfunc_ref;
	/* End - SRS-PLSQL-SUBPROC */
	/* Begin - ReqID:SRS-PLSQL-CURSOR */
	pstate->p_param_tupdesc_hook = plisql_param_tupdesc_ref;
	/* End - ReqID:SRS-PLSQL-CURSOR */
	/* Begin - ReqID:SRS-PLSQL-RECORD */
	pstate->p_orarecordconstruct_hook = plisql_ora_record_construct_ref;
	/* End - ReqID:SRS-PLSQL-RECORD */
	/* Begin - ReqID:SRS-PLSQL-Collection */
	pstate->p_indirection_hook = plisql_transform_indirection;
	/* End - ReqID:SRS-PLSQL-Collection */
	/* no need to use p_coerce_param_hook */
	pstate->p_ref_hook_state = (void *) expr;
}

/*
 * plisql_pre_column_ref		parser callback before parsing a ColumnRef
 */
static Node *
plisql_pre_column_ref(ParseState *pstate, ColumnRef *cref)
{
	PLiSQL_expr *expr = (PLiSQL_expr *) pstate->p_ref_hook_state;

	if (expr->func->resolve_option == PLISQL_RESOLVE_VARIABLE)
		return resolve_column_ref(pstate, expr, cref, false);
	else
		return NULL;
}

/*
 * plisql_post_column_ref		parser callback after parsing a ColumnRef
 */
static Node *
plisql_post_column_ref(ParseState *pstate, ColumnRef *cref, Node *var)
{
	PLiSQL_expr *expr = (PLiSQL_expr *) pstate->p_ref_hook_state;
	Node	   *myvar;

	if (expr->func->resolve_option == PLISQL_RESOLVE_VARIABLE)
		return NULL;			/* we already found there's no match */

	if (expr->func->resolve_option == PLISQL_RESOLVE_COLUMN && var != NULL)
		return NULL;			/* there's a table column, prefer that */

	/*
	 * If we find a record/row variable but can't match a field name, throw
	 * error if there was no core resolution for the ColumnRef either.  In
	 * that situation, the reference is inevitably going to fail, and
	 * complaining about the record/row variable is likely to be more on-point
	 * than the core parser's error message.  (It's too bad we don't have
	 * access to transformColumnRef's internal crerr state here, as in case of
	 * a conflict with a table name this could still be less than the most
	 * helpful error message possible.)
	 */
	myvar = resolve_column_ref(pstate, expr, cref, (var == NULL));

	if (myvar != NULL && var != NULL)
	{
		/*
		 * We could leave it to the core parser to throw this error, but we
		 * can add a more useful detail message than the core could.
		 */
		ereport(ERROR,
				(errcode(ERRCODE_AMBIGUOUS_COLUMN),
				 errmsg("column reference \"%s\" is ambiguous",
						NameListToString(cref->fields)),
				 errdetail("It could refer to either a PL/iSQL variable or a table column."),
				 parser_errposition(pstate, cref->location)));
	}

	return myvar;
}

/*
 * plisql_param_ref		parser callback for ParamRefs ($n symbols)
 */
static Node *
plisql_param_ref(ParseState *pstate, ParamRef *pref)
{
	PLiSQL_expr *expr = (PLiSQL_expr *) pstate->p_ref_hook_state;
	char		pname[32];
	PLiSQL_nsitem *nse;

	/* Begin - ReqID:SRS-SQL-LIBPQOUT */
	if (nodeTag(pref) == T_OraParamRef)
	{
		OraParamRef *oraref = (OraParamRef *) pref;

		if (expr->func->paramnames != NULL)
		{
			int i;

			for (i = 0; i < expr->func->fn_nargs; i++)
				if (expr->func->paramnames[i] != NULL &&
					strcmp(expr->func->paramnames[i], oraref->name) == 0)
			{
				/* Begin - BUG#M0000376 */
				if (enable_get_oraparam_mode && !pstate->p_not_transform_funcCall)
				{
					if (!pstate->p_not_set_oraparammode)
						set_oraparam_mode(oraref->name, PROARGMODE_IN);
					else if (pstate->p_oraparammode != ' ')
						set_oraparam_mode(oraref->name, pstate->p_oraparammode);
				}
				/* End - BUG#M0000376 */

				return make_datum_param(expr, expr->func->fn_argvarnos[i], oraref->location);
			}
		}
	}
	/* End - ReqID:SRS-SQL-LIBPQOUT */

	snprintf(pname, sizeof(pname), "$%d", pref->number);

	nse = plisql_ns_lookup(expr->ns, false,
							pname, NULL, NULL,
							NULL);

	if (nse == NULL)
		return NULL;			/* name not known to plisql */

	return make_datum_param(expr, nse->itemno, pref->location);
}

/*
 * resolve_column_ref		attempt to resolve a ColumnRef as a plisql var
 *
 * Returns the translated node structure, or NULL if name not found
 *
 * error_if_no_field tells whether to throw error or quietly return NULL if
 * we are able to match a record/row name but don't find a field name match.
 */
static Node *
resolve_column_ref(ParseState *pstate, PLiSQL_expr *expr,
				   ColumnRef *cref, bool error_if_no_field)
{
	PLiSQL_execstate *estate;
	PLiSQL_nsitem *nse;
	const char *name1;
	const char *name2 = NULL;
	const char *name3 = NULL;
	const char *colname = NULL;
	int			nnames;
	int			nnames_scalar = 0;
	int			nnames_wholerow = 0;
	int			nnames_field = 0;

	/*
	 * We use the function's current estate to resolve parameter data types.
	 * This is really pretty bogus because there is no provision for updating
	 * plans when those types change ...
	 */
	estate = expr->func->cur_estate;
	pstate->do_from_call = expr->func->do_from_call;	/* ReqID:SRS-SQL-CALL */

	/*----------
	 * The allowed syntaxes are:
	 *
	 * A		Scalar variable reference, or whole-row record reference.
	 * A.B		Qualified scalar or whole-row reference, or field reference.
	 * A.B.C	Qualified record field reference.
	 * A.*		Whole-row record reference.
	 * A.B.*	Qualified whole-row record reference.
	 *----------
	 */
	switch (list_length(cref->fields))
	{
		case 1:
			{
				Node	   *field1 = (Node *) linitial(cref->fields);

				Assert(IsA(field1, String));
				name1 = strVal(field1);
				nnames_scalar = 1;
				nnames_wholerow = 1;
				break;
			}
		case 2:
			{
				Node	   *field1 = (Node *) linitial(cref->fields);
				Node	   *field2 = (Node *) lsecond(cref->fields);

				Assert(IsA(field1, String));
				name1 = strVal(field1);

				/* Whole-row reference? */
				if (IsA(field2, A_Star))
				{
					/* Set name2 to prevent matches to scalar variables */
					name2 = "*";
					nnames_wholerow = 1;
					break;
				}

				Assert(IsA(field2, String));
				name2 = strVal(field2);
				colname = name2;
				nnames_scalar = 2;
				nnames_wholerow = 2;
				nnames_field = 1;
				break;
			}
		case 3:
			{
				Node	   *field1 = (Node *) linitial(cref->fields);
				Node	   *field2 = (Node *) lsecond(cref->fields);
				Node	   *field3 = (Node *) lthird(cref->fields);

				Assert(IsA(field1, String));
				name1 = strVal(field1);
				Assert(IsA(field2, String));
				name2 = strVal(field2);

				/* Whole-row reference? */
				if (IsA(field3, A_Star))
				{
					/* Set name3 to prevent matches to scalar variables */
					name3 = "*";
					nnames_wholerow = 2;
					break;
				}

				Assert(IsA(field3, String));
				name3 = strVal(field3);
				colname = name3;
				nnames_field = 2;
				break;
			}
		default:
			/*
			 * May be nested Oracle record variable, var.field1.field2...
			 */
			nnames_scalar = list_length(cref->fields);
			colname = strVal(list_nth(cref->fields, nnames_scalar - 1));
			nnames_field = nnames_scalar - 1;
			nnames_wholerow = nnames_scalar;
			name1 = strVal(list_nth(cref->fields, 0));
			name2 = strVal(list_nth(cref->fields, 1));;
			break;

	}

	nse = plisql_ns_lookup(expr->ns, false,
							name1, name2, name3,
							&nnames);

	/* Begin - ReqID:SRS-SQL-PACKAGE */
	/* maybe a package var */
	if (nse == NULL)
	{
		char		*refname;

		refname = NameListToString(cref->fields);

		nse = plisql_ns_lookup(expr->ns, false,
							refname, NULL, NULL,
							&nnames);
		pfree(refname);
		if (nse != NULL)
		{
			Assert(nse->itemtype == PLISQL_NSTYPE_VAR);
			return make_datum_param(expr, nse->itemno, cref->location);
		}
	}
	/* End - ReqID:SRS-SQL-PACKAGE */

	if (nse == NULL)
		return NULL;			/* name not known to plisql */

	switch (nse->itemtype)
	{
		case PLISQL_NSTYPE_VAR:
			if (nnames == nnames_scalar)
				return make_datum_param(expr, nse->itemno, cref->location);
			/* Begin - ReqID:SRS-PLSQL-CURSOR */
			else if (nnames == nnames_field &&
				is_oracursor_datum(estate->datums[nse->itemno]))
			{
				/* maybe a cursor attribute */
				PLiSQL_var *cursor= (PLiSQL_var *) estate->datums[nse->itemno];
				int			i;

				for (i = 0; i <= PLISQL_SUPPORT_ATTRIBUTE_NUM; i++)
				{
					PLiSQL_attribute *attr;

					attr = (PLiSQL_attribute *) estate->datums[cursor->firstattribute + i];

					Assert(attr->dtype == PLISQL_DTYPE_ATTRIBUTE &&
						   attr->attributeparentno == nse->itemno);
					if (strcmp(attr->fieldname, colname) == 0)
					{
						return make_datum_param(expr, i + cursor->firstattribute,
									cref->location);
					}
				}
				/*
				 * no attribute found
				 */
				if (error_if_no_field)
					ereport(ERROR,
							(errcode(ERRCODE_UNDEFINED_COLUMN),
							 errmsg("cursor \"%s\" has no attribute \"%s\"",
									(nnames_field == 1) ? name1 : name2,
									colname),
							 parser_errposition(pstate, cref->location)));
			}
			/* End - ReqID:SRS-PLSQL-CURSOR */
			break;
		case PLISQL_NSTYPE_REC:
			if (nnames == nnames_wholerow)
				return make_datum_param(expr, nse->itemno, cref->location);
			if (nnames == nnames_field)
			{
				/* colname could be a field in this record */
				PLiSQL_rec *rec = (PLiSQL_rec *) estate->datums[nse->itemno];
				int			i;

				/* search for a datum referencing this field */
				i = rec->firstfield;
				while (i >= 0)
				{
					PLiSQL_recfield *fld = (PLiSQL_recfield *) estate->datums[i];

					Assert(fld->dtype == PLISQL_DTYPE_RECFIELD &&
						   fld->recparentno == nse->itemno);
					if (strcmp(fld->fieldname, colname) == 0)
					{
						return make_datum_param(expr, i, cref->location);
					}
					i = fld->nextfield;
				}

				/*
				 * We should not get here, because a RECFIELD datum should
				 * have been built at parse time for every possible qualified
				 * reference to fields of this record.  But if we do, handle
				 * it like field-not-found: throw error or return NULL.
				 */
				if (error_if_no_field)
					ereport(ERROR,
							(errcode(ERRCODE_UNDEFINED_COLUMN),
							 errmsg("record \"%s\" has no field \"%s\"",
									(nnames_field == 1) ? name1 : name2,
									colname),
							 parser_errposition(pstate, cref->location)));
			}
			break;
		/* Begin - SRS-PLSQL-SUBPROC */
		case PLISQL_NSTYPE_SUBPROC_FUNC:
		case PLISQL_NSTYPE_SUBPROC_PROC:
			break;
		/* End - SRS-PLSQL-SUBPROC */
		/* Begin - ReqID:SRS-PLSQL-RECORD */
		case PLISQL_NSTYPE_ORA_RECORD:
			if (nnames == nnames_wholerow)
				return make_datum_param(expr, nse->itemno, cref->location);
			else if (nnames <= nnames_field)
			{
				/* colname could be a field in this Oralce record */
				PLiSQL_row *ora_rec = (PLiSQL_row *) estate->datums[nse->itemno];
				PLiSQL_datum *datum = NULL;

				/* Begin - BUG#Z1322 */
				/*
				 * check if the maximum number of layers that record nest
				 * is greater than PLISQL_MAXNUM_NESTED_RECORD
				 */
				plisql_check_maxnum_record_nest(list_length(cref->fields), nnames);
				/* End - BUG#Z1322 */

				if ((datum = plisql_parse_ora_rec_var(estate->datums, ora_rec, cref->fields, nnames, false)) != NULL)
					return make_datum_param(expr, datum->dno, cref->location);

				/* Not found, so throw error or return NULL */
				if (error_if_no_field)
					ereport(ERROR,
							(errcode(ERRCODE_UNDEFINED_COLUMN),
							 errmsg("record \"%s\" has no field \"%s\"",
									(nnames_field == 1) ? name1 : name2,
									colname),
							 parser_errposition(pstate, cref->location)));
			}
			break;
		/* End - ReqID:SRS-PLSQL-RECORD */
		default:
			elog(ERROR, "unrecognized plisql itemtype: %d", nse->itemtype);
	}

	/* Name format doesn't match the plisql variable type */
	return NULL;
}

/*
 * Helper for columnref parsing: build a Param referencing a plisql datum,
 * and make sure that that datum is listed in the expression's paramnos.
 */
static Node *
make_datum_param(PLiSQL_expr *expr, int dno, int location)
{
	PLiSQL_execstate *estate;
	PLiSQL_datum *datum;
	Param	   *param;
	MemoryContext oldcontext;

	/* see comment in resolve_column_ref */
	estate = expr->func->cur_estate;
	Assert(dno >= 0 && dno < estate->ndatums);
	datum = estate->datums[dno];

	/* Begin - ReqID:SRS-PLSQL-FORALL */
	if (expr->forallparams != NULL &&
		!expr->is_collect_subscript &&
		dno == expr->forallparams->indexdno)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("FORALL iteration variable %s is not allowed in this context",
					((PLiSQL_var *)datum)->refname)));
	/* End - ReqID:SRS-PLSQL-FORALL */

	/*
	 * Bitmapset must be allocated in function's permanent memory context
	 */
	oldcontext = MemoryContextSwitchTo(expr->func->fn_cxt);
	expr->paramnos = bms_add_member(expr->paramnos, dno);
	MemoryContextSwitchTo(oldcontext);

	param = makeNode(Param);
	param->paramkind = PARAM_EXTERN;
	param->paramid = dno + 1;
	plisql_exec_get_datum_type_info(estate,
									 datum,
									 &param->paramtype,
									 &param->paramtypmod,
									 &param->paramcollid,
									 &param->type); /* ReqID:SRS-PLSQL-CURSOR */
	param->location = location;

	return (Node *) param;
}


/* ----------
 * plisql_parse_word		The scanner calls this to postparse
 *				any single word that is not a reserved keyword.
 *
 * word1 is the downcased/dequoted identifier; it must be palloc'd in the
 * function's long-term memory context.
 *
 * yytxt is the original token text; we need this to check for quoting,
 * so that later checks for unreserved keywords work properly.
 *
 * We attempt to recognize the token as a variable only if lookup is true
 * and the plisql_IdentifierLookup context permits it.
 *
 * If recognized as a variable, fill in *wdatum and return true;
 * if not recognized, fill in *word and return false.
 * (Note: those two pointers actually point to members of the same union,
 * but for notational reasons we pass them separately.)
 * ----------
 */
bool
plisql_parse_word(char *paramname/* ReqID:SRS-SQL-ANONYOUT */,char *word1, const char *yytxt, bool lookup,
				   PLwdatum *wdatum, PLword *word)
{
	PLiSQL_nsitem *ns;

	/*
	 * We should not lookup variables in DECLARE sections.  In SQL
	 * expressions, there's no need to do so either --- lookup will happen
	 * when the expression is compiled.
	 */
	if (lookup && plisql_IdentifierLookup == IDENTIFIER_LOOKUP_NORMAL)
	{
		/*
		 * Do a lookup in the current namespace stack
		 */
		ns = plisql_ns_lookup(plisql_ns_top(), false,
							   paramname/* ReqID:SRS-SQL-ANONYOUT */, NULL, NULL,
							   NULL);

		if (ns != NULL)
		{
			switch (ns->itemtype)
			{
				case PLISQL_NSTYPE_VAR:
				case PLISQL_NSTYPE_REC:
				/* Begin - ReqID:SRS-PLSQL-RECORD */
				case PLISQL_NSTYPE_TYPE:
				case PLISQL_NSTYPE_ORA_RECORD:
				/* End - ReqID:SRS-PLSQL-RECORD */
					wdatum->datum = plisql_Datums[ns->itemno];
					wdatum->ident = word1;
					wdatum->quoted = (yytxt[0] == '"');
					/* Begin - ReqID:SRS-SQL-PACKAGE */
					wdatum->nname_used = 1;
					/* End - ReqID:SRS-SQL-PACKAGE */
					wdatum->idents = NIL;
					return true;
				/* Begin - SRS-PLSQL-SUBPROC */
				case PLISQL_NSTYPE_SUBPROC_FUNC:
				case PLISQL_NSTYPE_SUBPROC_PROC:
					break;
				/* End - SRS-PLSQL-SUBPROC */
				default:
					/* plisql_ns_lookup should never return anything else */
					elog(ERROR, "unrecognized plisql itemtype: %d",
						 ns->itemtype);
			}
		}
		/* Begin - ReqID:SRS-SQL-PACKAGE */
		else
		{
			/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
			if (!is_compile_standard_package())
			{
				/*
				 * handle var come from standard.xxx
				 * maybe a udt type construct func like udt_type(xx)
				 * so we should parse package entry
				 */
				PkgEntry *entry;
				size_t len = 8 + strlen(word1) + 2;
				char *refname = (char *) palloc0(len);
				List *idents =  list_make2(makeString("standard"),
							makeString(word1));

				snprintf(refname, len, "standard.%s", word1);

				entry = plisql_parse_package_entry(refname, -1, idents, true, true);
				pfree(refname);
				if (entry != NULL)
				{
					switch (entry->type)
					{
						case PKG_VAR:
							wdatum->datum = (PLiSQL_datum *) entry->value;
							wdatum->ident = word1;
							wdatum->quoted = false;
							wdatum->idents = NIL;
							wdatum->nname_used = 1;
							/* add namespace */
							if (plisql_ns_lookup(plisql_ns_top(), true,
								word1, NULL, NULL,
								NULL) == NULL)
								add_parameter_name(PLISQL_NSTYPE_VAR, wdatum->datum->dno, word1);
							return true;
							break;
						case PKG_TYPE:
							break;
						default:
							break;
					}
					pfree(entry);
				}
			}
			if (!is_compile_dbms_standard_package())
			{
				/*
				 * handle var come from dbms_standard.xxx
				 * maybe a udt type construct func like udt_type(xx)
				 * so we should parse package entry
				 */
				PkgEntry *entry;
				size_t len = 13 + strlen(word1) + 2;
				char *refname = (char *) palloc0(len);
				List *idents =  list_make2(makeString("dbms_standard"),
							makeString(word1));

				snprintf(refname, len, "dbms_standard.%s", word1);

				entry = plisql_parse_package_entry(refname, -1, idents, true, true);
				pfree(refname);
				if (entry != NULL)
				{
					switch (entry->type)
					{
						case PKG_VAR:
							wdatum->datum = (PLiSQL_datum *) entry->value;
							wdatum->ident = word1;
							wdatum->quoted = false;
							wdatum->idents = NIL;
							wdatum->nname_used = 1;
							/* add namespace */
							if (plisql_ns_lookup(plisql_ns_top(), true,
								word1, NULL, NULL,
								NULL) == NULL)
								add_parameter_name(PLISQL_NSTYPE_VAR, wdatum->datum->dno, word1);
							return true;
							break;
						case PKG_TYPE:
							break;
						default:
							break;
					}
					pfree(entry);
				}
			}
			/* End - ReqID:SRS-PLSQL-EXCEPTION */
		}
		/* End - ReqID:SRS-SQL-PACKAGE */
	}

	/* Begin - ReqID:SRS-SQL-PACKAGE */
	/*
	 * there, make sure the local namespace has not found or
	 * we lookup standard'package first than local, which will
	 * result mistake.
	 * for example: exit when not found which then found, we
	 * mustn't use standard'found
	 */
	if (lookup &&
		plisql_IdentifierLookup != IDENTIFIER_LOOKUP_DECLARE &&
		plisql_ns_lookup(plisql_ns_top(), false,
							   word1, NULL, NULL,
							   NULL) == NULL)
	{
		/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
		if (!is_compile_standard_package())
		{
			PkgEntry *entry;
			size_t len = 8 + strlen(word1) + 2;
			char *refname = (char *) palloc0(len);
			List *idents =  list_make2(makeString("standard"),
						makeString(word1));

			snprintf(refname, len, "standard.%s", word1);

			entry = plisql_parse_package_entry(refname, -1, idents, true, true);
			pfree(refname);
			if (entry != NULL)
			{
				switch (entry->type)
				{
					case PKG_VAR:
						wdatum->datum = (PLiSQL_datum *) entry->value;
						wdatum->ident = word1;
						wdatum->quoted = false;
						wdatum->idents = NIL;
						wdatum->nname_used = 1;
						/* add namespace */
						if (plisql_ns_lookup(plisql_ns_top(), true,
							word1, NULL, NULL,
							NULL) == NULL)
							add_parameter_name(PLISQL_NSTYPE_VAR,
											wdatum->datum->dno,
											word1);
						return true;
						break;
					case PKG_TYPE:
						break;
					default:
						break;
				}
				pfree(entry);
			}
		}
		if (!is_compile_dbms_standard_package())
		{
			PkgEntry *entry;
			size_t len = 13 + strlen(word1) + 2;
			char *refname = (char *) palloc0(len);
			List *idents =  list_make2(makeString("dbms_standard"),
						makeString(word1));

			snprintf(refname, len, "dbms_standard.%s", word1);

			entry = plisql_parse_package_entry(refname, -1, idents, true, true);
			pfree(refname);
			if (entry != NULL)
			{
				switch (entry->type)
				{
					case PKG_VAR:
						wdatum->datum = (PLiSQL_datum *) entry->value;
						wdatum->ident = word1;
						wdatum->quoted = false;
						wdatum->idents = NIL;
						wdatum->nname_used = 1;
						/* add namespace */
						if (plisql_ns_lookup(plisql_ns_top(), true,
							word1, NULL, NULL,
							NULL) == NULL)
							add_parameter_name(PLISQL_NSTYPE_VAR,
											wdatum->datum->dno,
											word1);
						return true;
						break;
					case PKG_TYPE:
						break;
					default:
						break;
				}
				pfree(entry);
			}
		}
		/* End - ReqID:SRS-PLSQL-EXCEPTION */
	}
	/* End - ReqID:SRS-SQL-PACKAGE */

	/*
	 * Nothing found - up to now it's a word without any special meaning for
	 * us.
	 */
	word->ident = word1;
	word->quoted = (yytxt[0] == '"');
	return false;
}


/* ----------
 * plisql_parse_dblword		Same lookup for two words
 *					separated by a dot.
 * ----------
 */
bool
plisql_parse_dblword(char *paramname/* ReqID:SRS-SQL-ANONYOUT */,char *word1, char *word2,
					  PLwdatum *wdatum, PLcword *cword)
{
	PLiSQL_nsitem *ns;
	List	   *idents;
	int			nnames;

	idents = list_make2(makeString(word1),
						makeString(word2));

	/*
	 * We should do nothing in DECLARE sections.  In SQL expressions, we
	 * really only need to make sure that RECFIELD datums are created when
	 * needed.  In all the cases handled by this function, returning a T_DATUM
	 * with a two-word idents string is the right thing.
	 */
	if (plisql_IdentifierLookup != IDENTIFIER_LOOKUP_DECLARE)
	{
		/*
		 * Do a lookup in the current namespace stack
		 */
		ns = plisql_ns_lookup(plisql_ns_top(), false,
							   paramname/* ReqID:SRS-SQL-ANONYOUT */, word2, NULL,
							   &nnames);
		if (ns != NULL)
		{
			switch (ns->itemtype)
			{
				case PLISQL_NSTYPE_VAR:
					/* Block-qualified reference to scalar variable. */
					wdatum->datum = plisql_Datums[ns->itemno];
					wdatum->ident = NULL;
					wdatum->quoted = false; /* not used */
					wdatum->idents = idents;
					/* Begin - ReqID:SRS-SQL-PACKAGE */
					wdatum->nname_used = nnames;
					/* End - ReqID:SRS-SQL-PACKAGE */
					return true;

				case PLISQL_NSTYPE_REC:
					if (nnames == 1)
					{
						/*
						 * First word is a record name, so second word could
						 * be a field in this record.  We build a RECFIELD
						 * datum whether it is or not --- any error will be
						 * detected later.
						 */
						PLiSQL_rec *rec;
						PLiSQL_recfield *new;

						rec = (PLiSQL_rec *) (plisql_Datums[ns->itemno]);
						new = plisql_build_recfield(rec, word2);

						wdatum->datum = (PLiSQL_datum *) new;
					}
					else
					{
						/* Block-qualified reference to record variable. */
						wdatum->datum = plisql_Datums[ns->itemno];
					}
					wdatum->ident = NULL;
					wdatum->quoted = false; /* not used */
					wdatum->idents = idents;
					/* Begin - ReqID:SRS-SQL-PACKAGE */
					wdatum->nname_used = nnames;
					/* End - ReqID:SRS-SQL-PACKAGE */
					return true;

				/* Begin - ReqID:SRS-PLSQL-RECORD */
				case PLISQL_NSTYPE_ORA_RECORD:
					if (nnames == 1)
					{
						/*
						 * First word is a Oracle record name, so second word could
						 * be a field in this record.  We build a
						 * datum if it is.
						 */
						PLiSQL_row	*ora_rec;
						int 		i;

						ora_rec = (PLiSQL_row *) (plisql_Datums[ns->itemno]);
						for (i = 0; i < ora_rec->nfields; i++)
						{
							if (ora_rec->fieldnames[i] &&
								pg_strcasecmp(ora_rec->fieldnames[i], word2) == 0)
							{
								wdatum->datum = plisql_Datums[ora_rec->varnos[i]];
								wdatum->ident = NULL;
								wdatum->quoted = false; /* not used */
								wdatum->idents = idents;
								/* Begin - ReqID:SRS-SQL-PACKAGE */
								wdatum->nname_used = nnames;
								/* End - ReqID:SRS-SQL-PACKAGE */
								return true;
							}
						}
					}
					else
					{
						/* Block-qualified reference to oracle record variable. */
						wdatum->datum = plisql_Datums[ns->itemno];
						/* Begin - BUG#Z1672 */
						wdatum->ident = NULL;
						wdatum->quoted = false; /* not used */
						wdatum->idents = idents;
						/* Begin - ReqID:SRS-SQL-PACKAGE */
						wdatum->nname_used = nnames;
						/* End - ReqID:SRS-SQL-PACKAGE */
						return true;
						/* End - BUG#Z1672 */
					}
					break;
				/* End - ReqID:SRS-PLSQL-RECORD */
				/* Begin - ReqID:SRS-PLSQL-Collection */
				case PLISQL_NSTYPE_TYPE:
					/* Block-qualified reference to udt type. */
					wdatum->datum = plisql_Datums[ns->itemno];
					wdatum->ident = NULL;
					wdatum->quoted = false; /* not used */
					wdatum->idents = idents;
					/* Begin - ReqID:SRS-SQL-PACKAGE */
					wdatum->nname_used = nnames;
					/* End - ReqID:SRS-SQL-PACKAGE */
					return true;
				/* End - ReqID:SRS-PLSQL-Collection */

				default:
					break;
			}
		}
		/* Begin - ReqID:SRS-SQL-PACKAGE */
		else
		{
			/*
			 * handle var come from package
			 * maybe a udt type construct func like udt_type(xx)
			 * so we should parse package entry
			 */
			PkgEntry *entry;
			size_t len = strlen(word1) + strlen(word2) + 2;
			char *refname = (char *) palloc0(len);

			snprintf(refname, len, "%s.%s", word1, word2);

			entry = plisql_parse_package_entry(refname, -1, idents, true, false);
			pfree(refname);
			if (entry != NULL)
			{
				switch (entry->type)
				{
					case PKG_VAR:
						wdatum->datum = (PLiSQL_datum *) entry->value;
						wdatum->ident = NULL;
						wdatum->quoted = false;
						wdatum->idents = idents;
						wdatum->nname_used = 2;
						return true;
						break;
					case PKG_TYPE:
						/* Begin - ReqID:SRS-PLSQL-Collection */
						wdatum->datum = NULL;
						wdatum->collectype = (PLiSQL_type *) entry->value;
						wdatum->ident = NULL;
						wdatum->quoted = false;
						wdatum->idents = idents;
						wdatum->nname_used = 2;
						return true;
						/* End - ReqID:SRS-PLSQL-Collection */
						break;
					default:
						break;
				}
				pfree(entry);
			}
		}
		/* End - ReqID:SRS-SQL-PACKAGE */
	}

	/* Nothing found */
	cword->idents = idents;
	return false;
}


/* ----------
 * plisql_parse_tripword		Same lookup for three words
 *					separated by dots.
 * ----------
 */
bool
plisql_parse_tripword(char *paramname/* ReqID:SRS-SQL-ANONYOUT */, char *word1, char *word2, char *word3,
					   PLwdatum *wdatum, PLcword *cword)
{
	PLiSQL_nsitem *ns;
	List	   *idents;
	int			nnames;

	/*
	 * We should do nothing in DECLARE sections.  In SQL expressions, we need
	 * to make sure that RECFIELD datums are created when needed, and we need
	 * to be careful about how many names are reported as belonging to the
	 * T_DATUM: the third word could be a sub-field reference, which we don't
	 * care about here.
	 */
	if (plisql_IdentifierLookup != IDENTIFIER_LOOKUP_DECLARE)
	{
		/*
		 * Do a lookup in the current namespace stack.  Must find a record
		 * reference, else ignore.
		 */
		ns = plisql_ns_lookup(plisql_ns_top(), false,
							   paramname/* ReqID:SRS-SQL-ANONYOUT */, word2, word3,
							   &nnames);
		if (ns != NULL)
		{
			switch (ns->itemtype)
			{
				case PLISQL_NSTYPE_REC:
					{
						PLiSQL_rec *rec;
						PLiSQL_recfield *new;

						rec = (PLiSQL_rec *) (plisql_Datums[ns->itemno]);
						if (nnames == 1)
						{
							/*
							 * First word is a record name, so second word
							 * could be a field in this record (and the third,
							 * a sub-field).  We build a RECFIELD datum
							 * whether it is or not --- any error will be
							 * detected later.
							 */
							new = plisql_build_recfield(rec, word2);
							idents = list_make2(makeString(word1),
												makeString(word2));
						}
						else
						{
							/* Block-qualified reference to record variable. */
							new = plisql_build_recfield(rec, word3);
							idents = list_make3(makeString(word1),
												makeString(word2),
												makeString(word3));
						}
						wdatum->datum = (PLiSQL_datum *) new;
						wdatum->ident = NULL;
						wdatum->quoted = false; /* not used */
						wdatum->idents = idents;
						/* Begin - ReqID:SRS-SQL-PACKAGE */
						wdatum->nname_used = nnames;
						/* End - ReqID:SRS-SQL-PACKAGE */
						return true;
					}

				/* Begin - ReqID:SRS-PLSQL-RECORD */
				case PLISQL_NSTYPE_ORA_RECORD:
					{
						PLiSQL_row	*ora_rec;
						int 		i;

						ora_rec = (PLiSQL_row *) (plisql_Datums[ns->itemno]);
						if (nnames == 1)
						{
							/*
							 * First word is a Oracle record name, so second word
							 * could be a field in this record (and the third,
							 * a sub-field). We build a filed
							 * datum if it is.
							 */
							for (i = 0; i < ora_rec->nfields; i++)
							{
								if (ora_rec->fieldnames[i] &&
									pg_strcasecmp(ora_rec->fieldnames[i], word2) == 0)
								{

									PLiSQL_row	*ora_rec2;
									PLiSQL_datum 	*datum;

									datum = plisql_Datums[ora_rec->varnos[i]];
									if (datum->dtype == PLISQL_DTYPE_ORA_RECORD)
									{
										int		j;

										ora_rec2 = (PLiSQL_row *) datum;
										for (j = 0; j < ora_rec2->nfields; j++)
										{
											if (ora_rec2->fieldnames[j] &&
												pg_strcasecmp(ora_rec2->fieldnames[j], word3) == 0 &&
												plisql_peek() != '.')
											{
												idents = list_make3(makeString(word1),
																	makeString(word2),
																	makeString(word3));

												wdatum->datum = plisql_Datums[ora_rec2->varnos[j]];
												wdatum->ident = NULL;
												wdatum->quoted = false; /* not used */
												wdatum->idents = idents;
												/* Begin - ReqID:SRS-SQL-PACKAGE */
												wdatum->nname_used = nnames;
												/* End - ReqID:SRS-SQL-PACKAGE */
												return true;
											}
										}
									}
									else if (datum->dtype == PLISQL_DTYPE_REC)
									{
										PLiSQL_rec *rec;
										PLiSQL_recfield *new;

										rec = (PLiSQL_rec *) datum;

										/*
										 * Block-qualified reference to record variable.
										 * First word is oracle record, the seond word is a record name,
										 * so second word could be a field in this record (and the third,
										 * a sub-field).  We build a RECFIELD datum
										 * whether it is or not --- any error will be
										 * detected later.
										 */
										new = plisql_build_recfield(rec, word3);
										idents = list_make3(makeString(word1),
															makeString(word2),
															makeString(word3));

										wdatum->datum = (PLiSQL_datum *) new;
										wdatum->ident = NULL;
										wdatum->quoted = false; /* not used */
										wdatum->idents = idents;
										/* Begin - ReqID:SRS-SQL-PACKAGE */
										wdatum->nname_used = nnames;
										/* End - ReqID:SRS-SQL-PACKAGE */
										return true;
									}
								}
							}
						}
						else if (nnames == 2)
						{
							/*
							 * Block-qualified reference to Oracle record variable.
							 * First word is Block-qualified reference,
							 * second word is a Oralce record name, so third word could
							 * be a field in this record. We build a filed
							 * datum if it is.
							 */
							for (i = 0; i < ora_rec->nfields; i++)
							{
								if (ora_rec->fieldnames[i] &&
									pg_strcasecmp(ora_rec->fieldnames[i], word3) == 0 &&
									plisql_peek() != '.')
								{
									idents = list_make3(makeString(word1),
														makeString(word2),
														makeString(word3));

									wdatum->datum = plisql_Datums[ora_rec->varnos[i]];
									wdatum->ident = NULL;
									wdatum->quoted = false; /* not used */
									wdatum->idents = idents;
									/* Begin - ReqID:SRS-SQL-PACKAGE */
									wdatum->nname_used = nnames;
									/* End - ReqID:SRS-SQL-PACKAGE */
									return true;
								}
							}
						}
						else if (nnames == 3)
						{
							/* Block-qualified reference to Oracle record variable. */
							idents = list_make3(makeString(word1),
												makeString(word2),
												makeString(word3));

							wdatum->datum = plisql_Datums[ns->itemno];
							wdatum->ident = NULL;
							wdatum->quoted = false; /* not used */
							wdatum->idents = idents;
							/*  Begin - ReqID:SRS-GKC-PACKAGE_214 */
							wdatum->nname_used = nnames;
							/*  End - ReqID:SRS-GKC-PACKAGE_214 */
							return true;
						}
					}
					break;
				/* End - ReqID:SRS-PLSQL-RECORD */

				default:
					break;
			}
		}
		/* Begin - ReqID:SRS-SQL-PACKAGE */
		else
		{
			/*
			 * handle var come from package
			 * it maybe is udt_type constructor func
			 * so we should parse package entry
			 */
			PkgEntry *entry;
			size_t len = strlen(word1) + strlen(word2) + strlen(word3) + 3;
			char *refname = (char *) palloc0(len);

			snprintf(refname, len, "%s.%s.%s", word1, word2, word3);
			idents = list_make3(makeString(word1),
								makeString(word2),
								makeString(word3));
			entry = plisql_parse_package_entry(refname, -1, idents, true, false);
			pfree(refname);
			if (entry != NULL)
			{
				switch (entry->type)
				{
					case PKG_VAR:
						wdatum->datum = (PLiSQL_datum *) entry->value;
						wdatum->ident = NULL;
						wdatum->quoted = false; /* not used */
						wdatum->idents = idents;
						wdatum->nname_used = 3;
						break;
					case PKG_TYPE:
						/* Begin - ReqID:SRS-PLSQL-Collection */
						wdatum->datum = NULL;
						wdatum->collectype = (PLiSQL_type *) entry->value;
						wdatum->ident = NULL;
						wdatum->quoted = false;
						wdatum->idents = idents;
						wdatum->nname_used = 3;
						/* End - ReqID:SRS-PLSQL-Collection */
						break;
					default:
						break;
				}
				pfree(entry);
				return true;
			}
		}
		/* End - ReqID:SRS-SQL-PACKAGE */
	}

	/* Nothing found */
	idents = list_make3(makeString(word1),
						makeString(word2),
						makeString(word3));
	cword->idents = idents;
	return false;
}

/* Begin - ReqID:SRS-PLSQL-RECORD */
/*
 * like plisql_parse_tripword
 * but support more than three words
 */
bool
plisql_parse_moreword(List *idents, PLwdatum *wdatum,
					PLcword *cword, int pre_last_tok,
					int lloc, int *nnames)
{
	PLiSQL_nsitem *ns = NULL;
	int			list_size;
	char		*last_name;

	list_size = list_length(idents);
	last_name = strVal(list_nth(idents, list_size - 1));

	if (plisql_IdentifierLookup != IDENTIFIER_LOOKUP_DECLARE)
	{
		/*
		 * According to the token, do a lookup in the current namespace stack.
		 * Must find a qualified reference, else ignore.
		 */
		if (pre_last_tok == '.')
			ns = plisql_ns_lookup_wordlist(plisql_ns_top(), false,
											idents,
											nnames);
		else if (pre_last_tok == '%')
		{
			ns = plisql_ns_lookup_wordlist(plisql_ns_top(), false,
									idents,
									nnames);

			if (*nnames != list_size - 1)
				ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					errmsg("identifier \'%s\' must be declared", last_name),
					plisql_scanner_errposition(lloc)));

			if (strcmp(last_name, "isopen") != 0 &&
				strcmp(last_name, "found") != 0 &&
				strcmp(last_name, "notfound") != 0 &&
				strcmp(last_name, "rowcount") != 0)
				ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					errmsg("identifier \'%s\' is not a legal cursor attribute", last_name),
					plisql_scanner_errposition(lloc)));
		}

		if (ns != NULL && *nnames == list_size - 1)
		{
			switch (ns->itemtype)
			{
				case PLISQL_NSTYPE_REC:
					{
						PLiSQL_rec *rec;
						PLiSQL_recfield *new;

						rec = (PLiSQL_rec *) (plisql_Datums[ns->itemno]);
						if (*nnames == 1)
						{
							/*
							 * First word is a record name, so second word
							 * could be a field in this record (and the third,
							 * a sub-field).  We build a RECFIELD datum
							 * whether it is or not --- any error will be
							 * detected later.
							 */
							new = plisql_build_recfield(rec, strVal(list_nth(idents, 1)));
						}
						else
						{
							/* Block-qualified reference to record variable. */
							new = plisql_build_recfield(rec, strVal(list_nth(idents, 1)));
							idents = list_make3(list_nth(idents, 0),
												list_nth(idents, 1),
												list_nth(idents, 2));

						}
						wdatum->datum = (PLiSQL_datum *) new;
						wdatum->ident = NULL;
						wdatum->quoted = false; /* not used */
						wdatum->idents = idents;
						/* Begin - ReqID:SRS-SQL-PACKAGE */
						wdatum->nname_used = *nnames;
						/* End - ReqID:SRS-SQL-PACKAGE */
						return true;
					}
					break;
				case PLISQL_NSTYPE_ORA_RECORD:
					{
						/*
						 * The penultimate word is a Oracle record name,
						 * so the last word
						 * could be a field in this record.
						 */
						PLiSQL_row 	*ora_rec = (PLiSQL_row *)plisql_Datums[ns->itemno];
						int		i;

						for (i = 0; i < ora_rec->nfields; i++)
						{
							if (ora_rec->varnos[i] < 0)
								continue;
							if (strcmp(ora_rec->fieldnames[i], last_name) == 0)
							{
								PLiSQL_datum	*datum = plisql_Datums[ora_rec->varnos[i]];

								wdatum->datum = datum;
								wdatum->ident = NULL;
								wdatum->quoted = false; /* not used */
								wdatum->idents = idents;
								wdatum->nname_used = *nnames;

								return true;
							}
						}

						elog(ERROR, "\"%s\" doesn't in record variable\"%s\"", last_name,
												ora_rec->refname);
					}
					break;

				case PLISQL_NSTYPE_SUBPROC_FUNC:
				case PLISQL_NSTYPE_SUBPROC_PROC:
					break;
				default:
					break;
			}
		}
		else if (ns != NULL && ns->itemtype == PLISQL_NSTYPE_ORA_RECORD)
		{
			wdatum->datum = plisql_Datums[ns->itemno];
			wdatum->ident = NULL;
			wdatum->quoted = false;	/* not used */
			wdatum->idents = idents;
			wdatum->nname_used = *nnames;

			return true;
		}
		else if (ns != NULL && *nnames == list_size &&
			(ns->itemtype == PLISQL_NSTYPE_VAR || ns->itemtype == PLISQL_NSTYPE_REC))
		{
			wdatum->datum = plisql_Datums[ns->itemno];
			wdatum->ident = NULL;
			wdatum->quoted = false;	/* not used */
			wdatum->idents = idents;
			wdatum->nname_used = *nnames;

			return true;
		}
		else if (ns != NULL && ns->itemtype == PLISQL_NSTYPE_REC)
		{
			PLiSQL_rec *rec;
			PLiSQL_recfield *new;

			rec = (PLiSQL_rec *) (plisql_Datums[ns->itemno]);
			if (*nnames == 1)
			{
				/*
				 * First word is a record name, so second word
				 * could be a field in this record (and the third,
				 * a sub-field).  We build a RECFIELD datum
				 * whether it is or not --- any error will be
				 * detected later.
				 */
				new = plisql_build_recfield(rec, strVal(list_nth(idents, *nnames)));
			}
			else
			{
				/* Block-qualified reference to record variable. */
				new = plisql_build_recfield(rec, strVal(list_nth(idents, *nnames)));
				idents = list_delete_nth_cell(idents, list_length(idents) - 1);

			}
			wdatum->datum = (PLiSQL_datum *) new;
			wdatum->ident = NULL;
			wdatum->quoted = false; /* not used */
			wdatum->idents = idents;
			/* Begin - ReqID:SRS-SQL-PACKAGE */
			wdatum->nname_used = *nnames;
			/* End - ReqID:SRS-SQL-PACKAGE */
			return true;
		}
		/* Begin - ReqID:SRS-SQL-PACKAGE */
		else if (ns == NULL)
		{
			/*
			 * handle var that comes from a package
			 * maybe it is udt_type constructor func
			 * so we should parse package entry
			 */
			PkgEntry 		*entry;
			size_t len 		= list_length(idents);
			ListCell 		*lc;
			int				i;
			StringInfoData	ds_refname;

			initStringInfo(&ds_refname);
			i = 0;
			foreach(lc, idents)
			{
				if (i == 0)
					appendStringInfoString(&ds_refname, strVal((Value *)lfirst(lc)));
				else
				{
					appendStringInfoString(&ds_refname, ".");
					appendStringInfoString(&ds_refname, strVal((Value *)lfirst(lc)));
				}
				i++;
			}

			entry = plisql_parse_package_entry(ds_refname.data, -1, idents, true, false);
			pfree(ds_refname.data);
			if (entry != NULL)
			{
				switch (entry->type)
				{
					case PKG_VAR:
						wdatum->datum = (PLiSQL_datum *) entry->value;
						wdatum->ident = NULL;
						wdatum->quoted = false; /* not used */
						wdatum->idents = idents;
						wdatum->nname_used = len;
						break;
					case PKG_TYPE:
						/* Begin - ReqID:SRS-PLSQL-Collection */
						wdatum->datum = NULL;
						wdatum->collectype = (PLiSQL_type *) entry->value;
						wdatum->ident = NULL;
						wdatum->quoted = false;
						wdatum->idents = idents;
						wdatum->nname_used = len;
						/* End - ReqID:SRS-PLSQL-Collection */
						break;
					default:
						break;
				}
				pfree(entry);
				return true;
			}
		}
		/* End - ReqID:SRS-SQL-PACKAGE */

	}

	/* Nothing found */
	cword->idents = idents;
	return false;
}
/* End - ReqID:SRS-PLSQL-RECORD */

/* ----------
 * plisql_parse_wordtype	The scanner found word%TYPE. word can be
 *				a variable name or a basetype.
 *
 * Returns datatype struct, or NULL if no match found for word.
 * ----------
 */
PLiSQL_type *
plisql_parse_wordtype(char *ident)
{
	PLiSQL_type *dtype;
	PLiSQL_nsitem *nse;
	TypeName   *typeName;
	HeapTuple	typeTup;
	/* Begin - ReqID:SRS-SQL-PACKAGE */
	List		*idents;
	/* End - ReqID:SRS-SQL-PACKAGE */

	/*
	 * Do a lookup in the current namespace stack
	 */
	nse = plisql_ns_lookup(plisql_ns_top(), false,
							ident, NULL, NULL,
							NULL);

	if (nse != NULL)
	{
		switch (nse->itemtype)
		{
			case PLISQL_NSTYPE_VAR:
				/* Begin - ReqID:SRS-PLSQL-TYPE */
				/*
				 * Set the notnull attribute of variable to isnull attribute of dtype.
				 * For example, name has a NOT NULL constraint, surname inherits the NOT NULL constraint.
				 * DECLARE
				 *   name     VARCHAR(25) NOT NULL := 'Smith';
				 *   surname  name%TYPE;
				 * BEGIN
				 * END;
				 */
				dtype = ((PLiSQL_var *) (plisql_Datums[nse->itemno]))->datatype;
				dtype->notnull = ((PLiSQL_var *) (plisql_Datums[nse->itemno]))->notnull;
				return dtype;
				/* End - ReqID:SRS-PLSQL-TYPE */

				/* XXX perhaps allow REC/ROW here? */
			/* Begin - ReqID:SRS-SQL-PACKAGE */
			case PLISQL_NSTYPE_REC:
				return ((PLiSQL_rec *) (plisql_Datums[nse->itemno]))->datatype;
			/* End - ReqID:SRS-SQL-PACKAGE */
			/* Begin - ReqID:SRS-PLSQL-RECORD */
			case PLISQL_NSTYPE_ORA_RECORD:
				return ((PLiSQL_row *)(plisql_Datums[nse->itemno]))->datatype;
			/* End - ReqID:SRS-PLSQL-RECORD */

			default:
				return NULL;
		}
	}

	/* maybe type comes from standard package */
	if (!is_compile_standard_package())
	{
		idents = list_make2(makeString("standard"), makeString(ident));
		if ((dtype = plisql_parse_package_type(makeTypeNameFromNameList(idents),
				parse_by_var_type, true)) != NULL)
		{
			list_free(idents);
			return dtype;
		}
		list_free(idents);
	}
	/* End - ReqID:SRS-SQL-PACKAGE */
	/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
	/*
	 * maybe type comes from dbms_standard pacakge
	 */
	if (!is_compile_dbms_standard_package())
	{
		idents = list_make2(makeString("dbms_standard"), makeString(ident));
		if ((dtype = plisql_parse_package_type(makeTypeNameFromNameList(idents),
				parse_by_var_type, true)) != NULL)
		{
			list_free(idents);
			return dtype;
		}
		list_free(idents);
	}
	/* End - ReqID:SRS-PLSQL-EXCEPTION */

	/*
	 * Word wasn't found in the namespace stack. Try to find a data type with
	 * that name, but ignore shell types and complex types.
	 */
	typeName = makeTypeName(ident);
	typeTup = LookupTypeName(NULL, typeName, NULL, false);
	if (typeTup)
	{
		Form_pg_type typeStruct = (Form_pg_type) GETSTRUCT(typeTup);

		if (!typeStruct->typisdefined ||
			typeStruct->typrelid != InvalidOid)
		{
			ReleaseSysCache(typeTup);
			return NULL;
		}

		/* Begin - ReqID:SRS-PLSQL-TYPE */
		typeName->pct_type = true;
		typeName->row_type = false;
		/* End - ReqID:SRS-PLSQL-TYPE */

		dtype = build_datatype(typeTup, -1,
							   plisql_curr_compile->fn_input_collation,
							   typeName);

		ReleaseSysCache(typeTup);
		return dtype;
	}

	/*
	 * Nothing found - up to now it's a word without any special meaning for
	 * us.
	 */
	return NULL;
}


/* ----------
 * plisql_parse_cwordtype		Same lookup for compositeword%TYPE
 * ----------
 */
PLiSQL_type *
plisql_parse_cwordtype(List *idents)
{
	PLiSQL_type *dtype = NULL;
	PLiSQL_nsitem *nse;
	const char *fldname;
	Oid			classOid;
	HeapTuple	classtup = NULL;
	HeapTuple	attrtup = NULL;
	HeapTuple	typetup = NULL;
	Form_pg_class classStruct;
	Form_pg_attribute attrStruct;
	MemoryContext oldCxt;
	/* Begin - ReqID:SRS-PLSQL-TYPE */
	TypeName	*typeName;
	/* End - ReqID:SRS-PLSQL-TYPE */
	/* Begin - BUG#Z1316 */
	int	nnames = 0;
	/* End - BUG#Z1316 */

	/* Avoid memory leaks in the long-term function context */
	oldCxt = MemoryContextSwitchTo(plisql_compile_tmp_cxt);

	if (list_length(idents) == 2)
	{
		/*
		 * Do a lookup in the current namespace stack. We don't need to check
		 * number of names matched, because we will only consider scalar
		 * variables.
		 */
		nse = plisql_ns_lookup(plisql_ns_top(), false,
								strVal(linitial(idents)),
								strVal(lsecond(idents)),
								NULL,
								&nnames/* BUG#Z1316 */);

		if (nse != NULL && nse->itemtype == PLISQL_NSTYPE_VAR)
		{
			dtype = ((PLiSQL_var *) (plisql_Datums[nse->itemno]))->datatype;
			/* Begin - ReqID:SRS-PLSQL-TYPE */
			/*
			 * Set the notnull attribute of variable to isnull attribute of dtype.
			 * For example, name has a NOT NULL constraint, surname inherits the NOT NULL constraint.
			 * DECLARE
			 *	 name	  VARCHAR(25) NOT NULL := 'Smith';
			 *	 surname  name%TYPE;
			 * BEGIN
			 * END;
			 */
			dtype->notnull = ((PLiSQL_var *) (plisql_Datums[nse->itemno]))->notnull;
			/* End - ReqID:SRS-PLSQL-TYPE */

			goto done;
		}
		/* Begin - ReqID:SRS-PLSQL-RECORD */
		else if (nse != NULL && nse->itemtype == PLISQL_NSTYPE_ORA_RECORD)
		{
			/* Begin - BUG#Z1316 */
			PLiSQL_row *row = (PLiSQL_row *) (plisql_Datums[nse->itemno]);

			if (nnames == list_length(idents))
			{
				/* maybe a rec2.rec1%type, rec2.rec1 is a record */
				if (row->datatype != NULL)
				{
					dtype = row->datatype;
				}
				else if (row->rowtupdesc != NULL &&
					row->rowtupdesc->tdtypeid != InvalidOid)
				{
					dtype = plisql_build_datatype(row->rowtupdesc->tdtypeid, -1,
									InvalidOid, false);
				}
				else
					elog(ERROR, "variable type is not a record or rowvar");
				goto done;
			}
			else
			{
				PLiSQL_datum	*datum;

				/* it is rec.field%type, plisql_ns_lookup return the record */
				if ((datum = plisql_parse_ora_rec_var(plisql_Datums, row, idents, nnames, false)) != NULL)
				{
					switch (datum->dtype)
					{
						case PLISQL_DTYPE_VAR:
							dtype = ((PLiSQL_var *) datum)->datatype;
							dtype->notnull = ((PLiSQL_var *) datum)->notnull;
							break;
						case PLISQL_DTYPE_ORA_RECORD:
							dtype = ((PLiSQL_row *)datum)->datatype;
							break;
						default:
							elog(ERROR, "dtype % is not supported:", datum->dtype);
					}
					goto done;;
				}
			}
			/* End - BUG#Z1316 */
		}
		/* End - ReqID:SRS-PLSQL-RECORD */

		/* Begin - ReqID:SRS-SQL-PACKAGE */
		if (nse != NULL && nse->itemtype == PLISQL_NSTYPE_REC)
		{
			/* Begin - BUG#Z7698 */
			if (nnames == list_length(idents))
			/* End - BUG#Z7698 */
				dtype = ((PLiSQL_rec *) (plisql_Datums[nse->itemno]))->datatype;
			/* Begin - BUG#Z7698 */
			else if (nnames == 1)
				dtype = get_row_field_type(((PLiSQL_rec *)(plisql_Datums[nse->itemno]))->datatype,
									strVal(lsecond(idents)));
			/* End - BUG#Z7698 */

			goto done;
		}
		/* maybe type comes from package */
		if ((dtype = plisql_parse_package_type(makeTypeNameFromNameList(idents),
				parse_by_var_type, false)) != NULL)
		{
			goto done;
		}
		/* End - ReqID:SRS-SQL-PACKAGE */

		/*
		 * First word could also be a table name
		 */
		classOid = RelnameGetRelid(strVal(linitial(idents)));
		/* Begin - OA-2465 */
		/* Check for synonyms */
		if (!OidIsValid(classOid) && SynnameGetRelname_hook)
		{
			List		*synoids = NIL;
			char *objschema;
			char *objname;
			char *objdblnksrv;
			Oid 		synoid;
			Oid 		nspid;

			synoid = SynnameGetRelname_hook(strVal(linitial(idents)), NULL, &objschema, &objname, &objdblnksrv, &synoids);
			if (OidIsValid(synoid))
			{
				nspid = LookupExplicitNamespace(objschema, true);
				if (OidIsValid(nspid))
					classOid = get_relname_relid(objname, nspid);
			}
		}
		/* End - OA-2465 */
		if (!OidIsValid(classOid))
			goto done;
		fldname = strVal(lsecond(idents));
	}
	else
	{
		/* Begin - ReqID:SRS-PLSQL-RECORD */
		int			nnames;

		/* support label1.label2.xxx.xxx%type */
		nse = plisql_ns_lookup_wordlist(plisql_ns_top(), false,
										idents,
										&nnames);
		if (nse != NULL)
		{
			if (nse->itemtype == PLISQL_NSTYPE_VAR)
			{
				dtype = ((PLiSQL_var *) (plisql_Datums[nse->itemno]))->datatype;
			}
			else if (nse->itemtype == PLISQL_NSTYPE_ORA_RECORD)
			{
				PLiSQL_row *row = (PLiSQL_row *) (plisql_Datums[nse->itemno]);

				if (nnames == list_length(idents))
				{
					/* maybe a rowvar%type */
					if (row->datatype != NULL)
					{
						dtype = row->datatype;
					}
					else if (row->rowtupdesc != NULL &&
						row->rowtupdesc->tdtypeid != InvalidOid)
					{
						dtype = plisql_build_datatype(row->rowtupdesc->tdtypeid, -1,
										InvalidOid, false);
					}
					else
						elog(ERROR, "variable type is not a record or rowvar");
				}
				/* Begin - BUG#Z1316 */
				else
				{
					PLiSQL_datum	*datum;

					/* it is rec.field%type, plisql_ns_lookup return the record */
					if ((datum = plisql_parse_ora_rec_var(plisql_Datums, row, idents, nnames, false)) != NULL)
					{
						switch (datum->dtype)
						{
							case PLISQL_DTYPE_VAR:
								dtype = ((PLiSQL_var *) datum)->datatype;
								dtype->notnull = ((PLiSQL_var *) datum)->notnull;
								break;
							case PLISQL_DTYPE_ORA_RECORD:
								dtype = ((PLiSQL_row *)datum)->datatype;
								break;
							default:
								elog(ERROR, "dtype % is not supported:", datum->dtype);
						}
						goto done;;
					}
				}
				/* End - BUG#Z1316 */
			}
			goto done;
		}
		else
		{
		/* End - ReqID:SRS-PLSQL-RECORD */
			RangeVar   *relvar;

			/* Begin - ReqID:SRS-SQL-PACKAGE */
			if ((dtype = plisql_parse_package_type(makeTypeNameFromNameList(idents),
								parse_by_var_type, false)) != NULL)
			{
				goto done;
			}
			/* End - ReqID:SRS-SQL-PACKAGE */

			relvar = makeRangeVar(strVal(linitial(idents)),
								  strVal(lsecond(idents)),
								  -1);
			/* Can't lock relation - we might not have privileges. */
			classOid = RangeVarGetRelid(relvar, NoLock, true);
			if (!OidIsValid(classOid))
				goto done;
			fldname = strVal(lthird(idents));
		}
	}

	classtup = SearchSysCache1(RELOID, ObjectIdGetDatum(classOid));
	if (!HeapTupleIsValid(classtup))
		goto done;
	classStruct = (Form_pg_class) GETSTRUCT(classtup);

	/*
	 * It must be a relation, sequence, view, materialized view, composite
	 * type, or foreign table
	 */
	if (classStruct->relkind != RELKIND_RELATION &&
		classStruct->relkind != RELKIND_SEQUENCE &&
		classStruct->relkind != RELKIND_VIEW &&
		classStruct->relkind != RELKIND_MATVIEW &&
		classStruct->relkind != RELKIND_COMPOSITE_TYPE &&
		classStruct->relkind != RELKIND_FOREIGN_TABLE &&
		classStruct->relkind != RELKIND_PARTITIONED_TABLE)
		goto done;

	/*
	 * Fetch the named table field and its type
	 */
	attrtup = SearchSysCacheAttName(classOid, fldname);
	if (!HeapTupleIsValid(attrtup))
		goto done;
	attrStruct = (Form_pg_attribute) GETSTRUCT(attrtup);

	typetup = SearchSysCache1(TYPEOID,
							  ObjectIdGetDatum(attrStruct->atttypid));
	if (!HeapTupleIsValid(typetup))
		elog(ERROR, "cache lookup failed for type %u", attrStruct->atttypid);

	/*
	 * Found that - build a compiler type struct in the caller's cxt and
	 * return it.  Note that we treat the type as being found-by-OID; no
	 * attempt to re-look-up the type name will happen during invalidations.
	 */
	MemoryContextSwitchTo(oldCxt);

	/* Begin - ReqID:SRS-PLSQL-TYPE */
	typeName = makeTypeNameFromNameList(idents);
	typeName->pct_type = true;
	typeName->row_type = false;
	/* End - ReqID:SRS-PLSQL-TYPE */

	dtype = build_datatype(typetup,
						   attrStruct->atttypmod,
						   attrStruct->attcollation,
						   typeName/* ReqID:SRS-PLSQL-TYPE */);

	/* Begin - ReqID:SRS-PLSQL-TYPE */
	/*
	 * Declare a variable which datatype is table.column%TYPE
	 * or schema.table.column%TYPE in a function, procedure or package,
	 * record the table or view OID, and column number(or 0 if not used).
	 */
	if (check_referenced_objects)
	{
		ObjectAddress *refobj;

		refobj = (ObjectAddress *)palloc(sizeof(ObjectAddress));
		refobj->classId = RelationRelationId;
		refobj->objectId = classOid;
		refobj->objectSubId = attrStruct->attnum;
		plisql_referenced_objects = lappend(plisql_referenced_objects, (void *)refobj);
	}
	/* End - ReqID:SRS-PLSQL-TYPE */

	MemoryContextSwitchTo(plisql_compile_tmp_cxt);

done:
	if (HeapTupleIsValid(classtup))
		ReleaseSysCache(classtup);
	if (HeapTupleIsValid(attrtup))
		ReleaseSysCache(attrtup);
	if (HeapTupleIsValid(typetup))
		ReleaseSysCache(typetup);

	MemoryContextSwitchTo(oldCxt);
	return dtype;
}

/* ----------
 * plisql_parse_wordrowtype		Scanner found word%ROWTYPE.
 *					So word must be a table name.
 * ----------
 */
PLiSQL_type *
plisql_parse_wordrowtype(char *ident)
{
	Oid			classOid;
	Oid			typOid;
	/* Begin - ReqID:SRS-SQL-PACKAGE */
	PLiSQL_type	*result;
	TypeName	*typname;
	bool		missing_ok = false;
	/* End - ReqID:SRS-SQL-PACKAGE */
	/* Begin - ReqID:SRS-PLSQL-TYPE */
	TypeName	*typeName;
	/* End - ReqID:SRS-PLSQL-TYPE */

	/* Begin - ReqID:SRS-PLSQL-CURSOR */
	/* look locale cursor%rowtype */
	result = plisql_parse_udt_rowtype(ident, true);
	if (result != NULL)
		return result;
	/* End - ReqID:SRS-PLSQL-CURSOR */

	/* Begin - ReqID:SRS-SQL-PACKAGE */
	typname = typeStringToTypeName(ident);
	/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
	if (list_length(typname->names) < 2)
	{
		pfree(typname);
		if (!is_compile_standard_package())
		{
			/* may be a standard.type */
			size_t len = 8 + strlen(ident) + 2;
			char *refname = (char *) palloc0(len);

			snprintf(refname, len, "standard.%s", ident);
			typname = typeStringToTypeName(refname);
			pfree(refname);
			missing_ok = true;
			result = plisql_parse_package_type(typname, parse_by_var_rowtype, missing_ok);
			pfree(typname);
			if (result != NULL)
				return result;
		}

		/* lookup dbms_standard package */
		if (!is_compile_dbms_standard_package())
		{
			/* may be a standard.type */
			size_t len = 13 + strlen(ident) + 2;
			char *refname = (char *) palloc0(len);

			snprintf(refname, len, "dbms_standard.%s", ident);
			typname = typeStringToTypeName(refname);
			pfree(refname);
			missing_ok = true;
			result = plisql_parse_package_type(typname, parse_by_var_rowtype, missing_ok);
			pfree(typname);
			if (result != NULL)
				return result;
		}
	}
	else
		pfree(typname);
	/* End - ReqID:SRS-PLSQL-EXCEPTION */
	/* End - ReqID:SRS-SQL-PACKAGE */

	/*
	 * Look up the relation.  Note that because relation rowtypes have the
	 * same names as their relations, this could be handled as a type lookup
	 * equally well; we use the relation lookup code path only because the
	 * errors thrown here have traditionally referred to relations not types.
	 * But we'll make a TypeName in case we have to do re-look-up of the type.
	 */
	classOid = RelnameGetRelid(ident);
	/* Begin - OA-2465 */
	/* Check for synonyms */
	if (!OidIsValid(classOid) && SynnameGetRelname_hook)
	{
		List		*synoids = NIL;
		char *objschema;
		char *objname;
		char *objdblnksrv;
		Oid 		synoid;
		Oid 		nspid;

		synoid = SynnameGetRelname_hook(ident, NULL, &objschema, &objname, &objdblnksrv, &synoids);
		if (OidIsValid(synoid))
		{
			nspid = LookupExplicitNamespace(objschema, missing_ok);
			if (OidIsValid(nspid))
				classOid = get_relname_relid(objname, nspid);
		}
	}
	/* End - OA-2465 */
	if (!OidIsValid(classOid))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_TABLE),
				 errmsg("relation \"%s\" does not exist", ident)));

	/* Some relkinds lack type OIDs */
	typOid = get_rel_type_id(classOid);
	if (!OidIsValid(typOid))
		ereport(ERROR,
				(errcode(ERRCODE_WRONG_OBJECT_TYPE),
				 errmsg("relation \"%s\" does not have a composite type",
						ident)));

	/* Begin - ReqID:SRS-PLSQL-TYPE */
	/*
	 * Declare a variable which datatype is table%ROWTYPE
	 * or schema.table%ROWTYPE in a function, procedure or package,
	 * record the table or view OID.
	 */
	if (check_referenced_objects)
	{
		ObjectAddress *refobj;

		refobj = (ObjectAddress *)palloc(sizeof(ObjectAddress));
		refobj->classId = RelationRelationId;
		refobj->objectId = classOid;
		refobj->objectSubId = InvalidAttrNumber;
		plisql_referenced_objects = lappend(plisql_referenced_objects, (void *)refobj);
	}
	/* End - ReqID:SRS-PLSQL-TYPE */

	/* Begin - ReqID:SRS-PLSQL-TYPE */
	typeName = makeTypeName(ident);
	typeName->pct_type = false;
	typeName->row_type = true;
	/* End - ReqID:SRS-PLSQL-TYPE */

	/* Build and return the row type struct */
	return plisql_build_datatype(typOid, -1, InvalidOid,
								  typeName/* ReqID:SRS-PLSQL-TYPE */);
}

/* ----------
 * plisql_parse_cwordrowtype		Scanner found compositeword%ROWTYPE.
 *			So word must be a namespace qualified table name.
 * ----------
 */
PLiSQL_type *
plisql_parse_cwordrowtype(List *idents)
{
	Oid			classOid;
	Oid			typOid;
	RangeVar   *relvar;
	MemoryContext oldCxt;
	/* Begin - ReqID:SRS-SQL-PACKAGE */
	PLiSQL_type	*result;
	/* End - ReqID:SRS-SQL-PACKAGE */
	/* Begin - ReqID:SRS-PLSQL-TYPE */
	TypeName	*typeName;
	/* End - ReqID:SRS-PLSQL-TYPE */
	/* Begin - ReqID:SRS-PLSQL-CURSOR */
	StringInfoData	refname;
	ListCell	*lc;
	/* End - ReqID:SRS-PLSQL-CURSOR */

	/* Begin - ReqID:SRS-PLSQL-CURSOR */
	/* lookup cursor%rowtype */
	initStringInfo(&refname);
	foreach(lc, idents)
		appendStringInfo(&refname, "%s.", strVal(lfirst(lc)));
	refname.data[refname.len - 1] = '\0';

	result = plisql_parse_udt_rowtype(refname.data, true);
	if (result != NULL)
	{
		pfree(refname.data);
		return result;
	}
	pfree(refname.data);
	/* End - ReqID:SRS-PLSQL-CURSOR */

	/* Begin - ReqID:SRS-SQL-PACKAGE */
	result = plisql_parse_package_type(makeTypeNameFromNameList(idents),
							parse_by_var_rowtype, false);
	if (result != NULL)
		return result;
	/* End - ReqID:SRS-SQL-PACKAGE */

	/*
	 * As above, this is a relation lookup but could be a type lookup if we
	 * weren't being backwards-compatible about error wording.
	 */
	if (list_length(idents) != 2)
		return NULL;

	/* Avoid memory leaks in long-term function context */
	oldCxt = MemoryContextSwitchTo(plisql_compile_tmp_cxt);

	/* Look up relation name.  Can't lock it - we might not have privileges. */
	relvar = makeRangeVar(strVal(linitial(idents)),
						  strVal(lsecond(idents)),
						  -1);
	classOid = RangeVarGetRelid(relvar, NoLock, false);

	/* Some relkinds lack type OIDs */
	typOid = get_rel_type_id(classOid);
	if (!OidIsValid(typOid))
		ereport(ERROR,
				(errcode(ERRCODE_WRONG_OBJECT_TYPE),
				 errmsg("relation \"%s\" does not have a composite type",
						strVal(lsecond(idents)))));

	MemoryContextSwitchTo(oldCxt);

	/* Begin - ReqID:SRS-PLSQL-TYPE */
	/*
	 * Declare a variable which datatype is schema.table%ROWTYPE
	 * in a function, procedure or package,
	 * record the table or view OID.
	 */
	if (check_referenced_objects)
	{
		ObjectAddress *refobj;

		refobj = (ObjectAddress *)palloc(sizeof(ObjectAddress));
		refobj->classId = RelationRelationId;
		refobj->objectId = classOid;
		refobj->objectSubId = InvalidAttrNumber;
		plisql_referenced_objects = lappend(plisql_referenced_objects, (void *)refobj);
	}

	/* End - ReqID:SRS-PLSQL-TYPE */

	/* Begin - ReqID:SRS-PLSQL-TYPE */
	typeName = makeTypeNameFromNameList(idents);
	typeName->pct_type = false;
	typeName->row_type = true;
	/* End - ReqID:SRS-PLSQL-TYPE */

	/* Build and return the row type struct */
	return plisql_build_datatype(typOid, -1, InvalidOid,
								  typeName/* ReqID:SRS-PLSQL-TYPE */);
}

/*
 * plisql_build_variable - build a datum-array entry of a given
 * datatype
 *
 * The returned struct may be a PLiSQL_var or PLiSQL_rec
 * depending on the given datatype, and is allocated via
 * palloc.  The struct is automatically added to the current datum
 * array, and optionally to the current namespace.
 */
PLiSQL_variable *
plisql_build_variable(const char *refname, int lineno, PLiSQL_type *dtype,
					   bool add2namespace)
{
	PLiSQL_variable *result;

	switch (dtype->ttype)
	{
		case PLISQL_TTYPE_UDT_REF_CURSOR: /* ReqID:SRS-PLSQL-CURSOR */
		case PLISQL_TTYPE_SUBTYPE: /* ReqID:SRS-PLSQL-SUBTYPE */
		case PLISQL_TTYPE_SCALAR:
		/* Begin - ReqID:SRS-PLSQL-Collection */
		case PLISQL_TTYPE_COLLECT_TYPE:
		/* End - ReqID:SRS-PLSQL-Collection */
			{
				/* Ordinary scalar datatype */
				PLiSQL_var *var;

				var = palloc0(sizeof(PLiSQL_var));
				var->dtype = PLISQL_DTYPE_VAR;
				var->refname = pstrdup(refname);
				var->lineno = lineno;
				var->datatype = dtype;
				/* other fields are left as 0, might be changed by caller */

				/* preset to NULL */
				var->value = 0;
				var->isnull = true;
				var->freeval = false;
				/* Begin - ReqID:SRS-SQL-PACKAGE */
				var->pkgoid = (plisql_compile_packageitem == NULL ? InvalidOid : plisql_compile_packageitem->source.fn_oid);
				/* End - ReqID:SRS-SQL-PACKAGE */

				plisql_adddatum((PLiSQL_datum *) var);
				/* Begin - ReqID:SRS-PLSQL-CURSOR */
				if (ISORACURSOROID(dtype->typoid))
					plisql_build_cursor_attribute(var);
				/* End - ReqID:SRS-PLSQL-CURSOR */
				if (add2namespace)
				{
					plisql_ns_additem(PLISQL_NSTYPE_VAR,
									   var->dno,
									   refname);
					/* Begin - ReqID:SRS-PLSQL-CURSOR */
					/*
					 * record oid to namespace, so that
					 * plisql_ns_lookup can found cursorvar
					 * attribute
					 */
					if (ISORACURSOROID(dtype->typoid))
					{
						PLiSQL_nsitem *nse;

						nse = plisql_ns_top();
						nse->typoid = dtype->typoid;
					}
					/* End - ReqID:SRS-PLSQL-CURSOR */
				}
				/* Begin - ReqID:SRS-PLSQL-CURSOR */
				if (dtype->ttype == PLISQL_TTYPE_UDT_REF_CURSOR)
				{
					PLiSQL_ref_cursor_type *cursor_type = (PLiSQL_ref_cursor_type *) dtype;

					var->cursor_rettype_tupledesc = cursor_type->rettype_tupdesc;
				}
				/* End - ReqID:SRS-PLSQL-CURSOR */
				/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
				if (ISEXCEPTIONOID(dtype->typoid))
				{
					var->self_define = true;
					var->errormsg = pstrdup(PLISQL_UDT_EXCEPTION_DEFAULT_ERRMSG);
					var->value = Int32GetDatum(PLISQL_UDT_EXCEPTION_DEFAULT_SQLCODE);
					var->oraerrcode = PLISQL_UDT_EXCEPTION_DEFAULT_SQLCODE;
					var->isnull = false;
				}
				/* End - ReqID:SRS-PLSQL-EXCEPTION */
				result = (PLiSQL_variable *) var;
				break;
			}
		case PLISQL_TTYPE_REC:
			{
				/* Composite type -- build a record variable */
				PLiSQL_rec *rec;

				rec = plisql_build_record(refname, lineno,
										   dtype, dtype->typoid,
										   add2namespace);
				/* Begin - ReqID:SRS-SQL-PACKAGE */
				rec->pkgoid = (plisql_compile_packageitem == NULL ? InvalidOid : plisql_compile_packageitem->source.fn_oid);
				/* End - ReqID:SRS-SQL-PACKAGE */

				result = (PLiSQL_variable *) rec;
				break;
			}
		case PLISQL_TTYPE_PSEUDO:
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("variable \"%s\" has pseudo-type %s",
							refname, format_type_be(dtype->typoid))));
			result = NULL;		/* keep compiler quiet */
			break;
		/* Begin - ReqID:SRS-PLSQL-CURSOR */
		case PLISQL_TTYPE_EXPLICIT_CURSOR:
			{
				/* Composite type -- build a record variable */
				PLiSQL_rec *rec;

				rec = plisql_build_record_for_cursorrowtype(refname, lineno,
															dtype, add2namespace);
				result = (PLiSQL_variable *) rec;
				break;
			}
		/* End - ReqID:SRS-PLSQL-CURSOR */
		/* Begin - ReqID:SRS-PLSQL-RECORD */
		case PLISQL_TTYPE_ORARECORD_TYPE:
			{
				/* Oracle compatible record type, -- build a row variable */
				PLiSQL_row *row;

				row = plisql_build_row_for_ora_record(refname, lineno,
												(PLiSQL_ora_record_type *)dtype,
												add2namespace);
				/* Begin - ReqID:SRS-SQL-PACKAGE */
				row->pkgoid = (plisql_compile_packageitem == NULL ? InvalidOid : plisql_compile_packageitem->source.fn_oid);
				/* End - ReqID:SRS-SQL-PACKAGE */

				result = (PLiSQL_variable *) row;
				break;
			}
		/* End - ReqID:SRS-PLSQL-RECORD */
		default:
			elog(ERROR, "unrecognized ttype: %d", dtype->ttype);
			result = NULL;		/* keep compiler quiet */
			break;
	}

	return result;
}

/*
 * Build empty named record variable, and optionally add it to namespace
 */
PLiSQL_rec *
plisql_build_record(const char *refname, int lineno,
					 PLiSQL_type *dtype, Oid rectypeid,
					 bool add2namespace)
{
	PLiSQL_rec *rec;

	rec = palloc0(sizeof(PLiSQL_rec));
	rec->dtype = PLISQL_DTYPE_REC;
	rec->refname = pstrdup(refname);
	rec->lineno = lineno;
	/* other fields are left as 0, might be changed by caller */
	rec->datatype = dtype;
	rec->rectypeid = rectypeid;
	rec->firstfield = -1;
	rec->erh = NULL;
	plisql_adddatum((PLiSQL_datum *) rec);
	if (add2namespace)
		plisql_ns_additem(PLISQL_NSTYPE_REC, rec->dno, rec->refname);

	/* Begin - ReqID:SRS-SQL-PACKAGE */
	rec->pkgoid = (plisql_compile_packageitem == NULL ? InvalidOid : plisql_compile_packageitem->source.fn_oid);
	/* Begin - BUG#M0000168 */
	plisql_expand_rec_field(rec);
	/* End - BUG#M0000168 */

	return rec;
}

/*
 * Build a row-variable data structure given the component variables.
 * Include a rowtupdesc, since we will need to materialize the row result.
 */
PLiSQL_row *
build_row_from_vars(PLiSQL_variable **vars, int numvars)
{
	PLiSQL_row *row;
	int			i;

	row = palloc0(sizeof(PLiSQL_row));
	row->dtype = PLISQL_DTYPE_ROW;
	row->refname = "(unnamed row)";
	row->lineno = -1;
	row->rowtupdesc = CreateTemplateTupleDesc(numvars, false);
	row->nfields = numvars;
	row->fieldnames = palloc(numvars * sizeof(char *));
	row->varnos = palloc(numvars * sizeof(int));

	for (i = 0; i < numvars; i++)
	{
		PLiSQL_variable *var = vars[i];
		Oid			typoid = InvalidOid;
		int32		typmod = 0;
		Oid			typcoll = InvalidOid;

		/* Member vars of a row should never be const */
		Assert(!var->isconst);

		switch (var->dtype)
		{
			case PLISQL_DTYPE_VAR:
			case PLISQL_DTYPE_PROMISE:
				typoid = ((PLiSQL_var *) var)->datatype->typoid;
				typmod = ((PLiSQL_var *) var)->datatype->atttypmod;
				typcoll = ((PLiSQL_var *) var)->datatype->collation;
				break;

			case PLISQL_DTYPE_REC:
				/* shouldn't need to revalidate rectypeid already... */
				typoid = ((PLiSQL_rec *) var)->rectypeid;
				typmod = -1;	/* don't know typmod, if it's used at all */
				typcoll = InvalidOid;	/* composite types have no collation */
				break;

			/* Begin - ReqID:SRS-PLSQL-RECORD */
			case PLISQL_DTYPE_ORA_RECORD:
				if (((PLiSQL_row *) var)->rowtupdesc)
				{
					typoid = ORARECORDOID;
					typmod = ((PLiSQL_row *) var)->rowtupdesc->tdtypmod;
					/* composite types have no collation */
				}
				break;
			/* End - ReqID:SRS-PLSQL-RECORD */

			default:
				elog(ERROR, "unrecognized dtype: %d", var->dtype);
				typoid = InvalidOid;	/* keep compiler quiet */
				typmod = 0;
				typcoll = InvalidOid;
				break;
		}

		row->fieldnames[i] = var->refname;
		row->varnos[i] = var->dno;

		TupleDescInitEntry(row->rowtupdesc, i + 1,
						   var->refname,
						   typoid, typmod,
						   0);
		TupleDescInitEntryCollation(row->rowtupdesc, i + 1, typcoll);
	}

	return row;
}

/*
 * Build a RECFIELD datum for the named field of the specified record variable
 *
 * If there's already such a datum, just return it; we don't need duplicates.
 */
PLiSQL_recfield *
plisql_build_recfield(PLiSQL_rec *rec, const char *fldname)
{
	PLiSQL_recfield *recfield;
	int			i;

	/* search for an existing datum referencing this field */
	i = rec->firstfield;
	while (i >= 0)
	{
		PLiSQL_recfield *fld;

		/* Begin - BUG#M0000168 */
		/*
		 * Pg record can only be used inside a function
		 */
		if ((rec->rectypeid == RECORDOID || !OidIsValid(rec->rectypeid)) &&
			plisql_nsubprocFuncs > 0)
		{
			if (i >= plisql_nDatums)
				elog(ERROR, "Pg record can only be used inside a function");
			fld = (PLiSQL_recfield *) plisql_Datums[i];
			if (fld->dtype != PLISQL_DTYPE_RECFIELD ||
				fld->recparentno != rec->dno)
				elog(ERROR, "Pg record can only be used inside a function");
		}
		else
		{
			fld = (PLiSQL_recfield *) plisql_Datums[i];

			Assert(fld->dtype == PLISQL_DTYPE_RECFIELD &&
			   fld->recparentno == rec->dno);
		}
		/* End - BUG#M0000168 */
		if (strcmp(fld->fieldname, fldname) == 0)
			return fld;
		i = fld->nextfield;
	}

	/* nope, so make a new one */
	recfield = palloc0(sizeof(PLiSQL_recfield));
	recfield->dtype = PLISQL_DTYPE_RECFIELD;
	recfield->fieldname = pstrdup(fldname);
	recfield->recparentno = rec->dno;
	recfield->rectupledescid = INVALID_TUPLEDESC_IDENTIFIER;
	/* Begin - ReqID:SRS-SQL-PACKAGE */
	recfield->pkgoid = (plisql_compile_packageitem == NULL ? InvalidOid : plisql_compile_packageitem->source.fn_oid);
	/* End - ReqID:SRS-SQL-PACKAGE */

	plisql_adddatum((PLiSQL_datum *) recfield);

	/* now we can link it into the parent's chain */
	recfield->nextfield = rec->firstfield;
	rec->firstfield = recfield->dno;

	return recfield;
}

/*
 * plisql_build_datatype
 *		Build PLiSQL_type struct given type OID, typmod, collation,
 *		and type's parsed name.
 *
 * If collation is not InvalidOid then it overrides the type's default
 * collation.  But collation is ignored if the datatype is non-collatable.
 *
 * origtypname is the parsed form of what the user wrote as the type name.
 * It can be NULL if the type could not be a composite type, or if it was
 * identified by OID to begin with (e.g., it's a function argument type).
 */
PLiSQL_type *
plisql_build_datatype(Oid typeOid, int32 typmod,
					   Oid collation, TypeName *origtypname)
{
	HeapTuple	typeTup;
	PLiSQL_type *typ;

	typeTup = SearchSysCache1(TYPEOID, ObjectIdGetDatum(typeOid));
	if (!HeapTupleIsValid(typeTup))
		elog(ERROR, "cache lookup failed for type %u", typeOid);

	typ = build_datatype(typeTup, typmod, collation, origtypname);

	ReleaseSysCache(typeTup);

	return typ;
}

/*
 * Utility subroutine to make a PLiSQL_type struct given a pg_type entry
 * and additional details (see comments for plisql_build_datatype).
 */
static PLiSQL_type *
build_datatype(HeapTuple typeTup, int32 typmod,
			   Oid collation, TypeName *origtypname)
{
	Form_pg_type typeStruct = (Form_pg_type) GETSTRUCT(typeTup);
	PLiSQL_type *typ;

	if (!typeStruct->typisdefined)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("type \"%s\" is only a shell",
						NameStr(typeStruct->typname))));

	typ = (PLiSQL_type *) palloc(sizeof(PLiSQL_type));

	typ->typname = pstrdup(NameStr(typeStruct->typname));
	typ->typoid = typeStruct->oid;
	switch (typeStruct->typtype)
	{
		case TYPTYPE_BASE:
		case TYPTYPE_ENUM:
		case TYPTYPE_RANGE:
		case TYPTYPE_MULTIRANGE:
			typ->ttype = PLISQL_TTYPE_SCALAR;
			break;
		case TYPTYPE_COMPOSITE:
			typ->ttype = PLISQL_TTYPE_REC;
			break;
		case TYPTYPE_DOMAIN:
			if (type_is_rowtype(typeStruct->typbasetype))
				typ->ttype = PLISQL_TTYPE_REC;
			else
				typ->ttype = PLISQL_TTYPE_SCALAR;
			break;
		case TYPTYPE_PSEUDO:
			if (typ->typoid == RECORDOID)
				typ->ttype = PLISQL_TTYPE_REC;
			/* Begin - ReqID:SRS-PLSQL-RECORD */
			else if (typ->typoid == ORARECORDOID)
				typ->ttype = PLISQL_TTYPE_ORARECORD_TYPE;
			/* End - ReqID:SRS-PLSQL-RECORD */
			else
				typ->ttype = PLISQL_TTYPE_PSEUDO;
			break;
		default:
			elog(ERROR, "unrecognized typtype: %d",
				 (int) typeStruct->typtype);
			break;
	}
	typ->typlen = typeStruct->typlen;
	typ->typbyval = typeStruct->typbyval;
	typ->typtype = typeStruct->typtype;
	typ->collation = typeStruct->typcollation;
	if (OidIsValid(collation) && OidIsValid(typ->collation))
		typ->collation = collation;
	/* Detect if type is true array, or domain thereof */
	/* NB: this is only used to decide whether to apply expand_array */
	if (typeStruct->typtype == TYPTYPE_BASE)
	{
		/*
		 * This test should include what get_element_type() checks.  We also
		 * disallow non-toastable array types (i.e. oidvector and int2vector).
		 */
		typ->typisarray = (IsTrueArrayType(typeStruct) &&
						   typeStruct->typstorage != TYPSTORAGE_PLAIN);
	}
	else if (typeStruct->typtype == TYPTYPE_DOMAIN)
	{
		/* we can short-circuit looking up base types if it's not varlena */
		typ->typisarray = (typeStruct->typlen == -1 &&
						   typeStruct->typstorage != TYPSTORAGE_PLAIN &&
						   OidIsValid(get_base_element_type(typeStruct->typbasetype)));
	}
	else
		typ->typisarray = false;
	typ->atttypmod = typmod;

	/*
	 * If it's a named composite type (or domain over one), find the typcache
	 * entry and record the current tupdesc ID, so we can detect changes
	 * (including drops).  We don't currently support on-the-fly replacement
	 * of non-composite types, else we might want to do this for them too.
	 */
	if (typ->ttype == PLISQL_TTYPE_REC && typ->typoid != RECORDOID)
	{
		TypeCacheEntry *typentry;

		typentry = lookup_type_cache(typ->typoid,
									 TYPECACHE_TUPDESC |
									 TYPECACHE_DOMAIN_BASE_INFO);
		if (typentry->typtype == TYPTYPE_DOMAIN)
			typentry = lookup_type_cache(typentry->domainBaseType,
										 TYPECACHE_TUPDESC);
		if (typentry->tupDesc == NULL)
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("type %s is not composite",
							format_type_be(typ->typoid))));

		typ->origtypname = origtypname;
		typ->tcache = typentry;
		typ->tupdesc_id = typentry->tupDesc_identifier;
	}
	else
	{
		typ->origtypname = NULL;
		typ->tcache = NULL;
		typ->tupdesc_id = 0;
	}

	/* Begin - ReqID:SRS-PLSQL-TYPE */
	/*
	 * If it's a %TYPE or %ROWTYPE type, we put the original type name in pctrowtypname.
	 */
	typ->notnull = false;
	if (origtypname!= NULL &&
		(origtypname->pct_type || origtypname->row_type))
		typ->pctrowtypname = origtypname;
	else
		typ->pctrowtypname = NULL;
	/* End - ReqID:SRS-PLSQL-TYPE */

	return typ;
}

/*
 *	plisql_recognize_err_condition
 *		Check condition name and translate it to SQLSTATE.
 *
 * Note: there are some cases where the same condition name has multiple
 * entries in the table.  We arbitrarily return the first match.
 */
int
plisql_recognize_err_condition(const char *condname, bool allow_sqlstate)
{
	int			i;

	if (allow_sqlstate)
	{
		if (strlen(condname) == 5 &&
			strspn(condname, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ") == 5)
			return MAKE_SQLSTATE(condname[0],
								 condname[1],
								 condname[2],
								 condname[3],
								 condname[4]);
	}

	for (i = 0; exception_label_map[i].label != NULL; i++)
	{
		if (strcmp(condname, exception_label_map[i].label) == 0)
			return exception_label_map[i].sqlerrstate;
	}

	ereport(ERROR,
			(errcode(ERRCODE_UNDEFINED_OBJECT),
			 errmsg("unrecognized exception condition \"%s\"",
					condname)));
	return 0;					/* keep compiler quiet */
}

/*
 * plisql_parse_err_condition
 *		Generate PLiSQL_condition entry(s) for an exception condition name
 *
 * This has to be able to return a list because there are some duplicate
 * names in the table of error code names.
 */
PLiSQL_condition *
plisql_parse_err_condition(char *condname)
{
	int			i;
	PLiSQL_condition *new;
	PLiSQL_condition *prev;

	/*
	 * XXX Eventually we will want to look for user-defined exception names
	 * here.
	 */

	/*
	 * OTHERS is represented as code 0 (which would map to '00000', but we
	 * have no need to represent that as an exception condition).
	 */
	if (strcmp(condname, "others") == 0)
	{
		new = palloc(sizeof(PLiSQL_condition));
		new->sqlerrstate = 0;
		new->condname = condname;
		new->next = NULL;
		/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
		new->excepdno = -1;
		/* End - ReqID:SRS-PLSQL-EXCEPTION */
		return new;
	}

	prev = NULL;
	for (i = 0; exception_label_map[i].label != NULL; i++)
	{
		if (strcmp(condname, exception_label_map[i].label) == 0)
		{
			new = palloc(sizeof(PLiSQL_condition));
			new->sqlerrstate = exception_label_map[i].sqlerrstate;
			new->condname = condname;
			new->next = prev;
			/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
			new->excepdno = -1;
			/* End - ReqID:SRS-PLSQL-EXCEPTION */
			prev = new;
		}
	}

	if (!prev)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("unrecognized exception condition \"%s\"",
						condname)));

	return prev;
}

/* ----------
 * plisql_start_datums			Initialize datum list at compile startup.
 * ----------
 */
void
plisql_start_datums(void)
{
	/* Begin - BUG#M0000168 */
	int pre_level = cur_compile_func_level - 1;

	if (pre_level < 0)
	{
		datums_alloc = 128;
		plisql_nDatums = 0;
		/* This is short-lived, so needn't allocate in function's cxt */
		plisql_Datums = MemoryContextAlloc(plisql_compile_tmp_cxt,
											sizeof(PLiSQL_datum *) * datums_alloc);
		/* datums_last tracks what's been seen by plisql_add_initdatums() */
		datums_last = 0;
	}
	else
	{
		int	pre_datums_alloc = plisql_saved_datums_alloc[pre_level];
		int pre_nDatums = plisql_saved_nDatums[pre_level];
		int pre_datums_last = plisql_saved_datums_last[pre_level];
		int i;
		PLiSQL_datum **pre_Datums = plisql_saved_datums[pre_level];

		/* handle datums_alloc */
		if (pre_datums_alloc <= 128)
			datums_alloc = 128;
		else
		{
			datums_alloc = 128;
			while (datums_alloc < pre_datums_alloc)
				datums_alloc = datums_alloc * 2;
		}
		plisql_Datums = MemoryContextAlloc(plisql_compile_tmp_cxt,
											sizeof(PLiSQL_datum *) * datums_alloc);
		for (i = 0; i < pre_nDatums; i++)
			plisql_Datums[i] = pre_Datums[i];
		plisql_nDatums = pre_nDatums;
		datums_last = pre_datums_last + 1;
	}
	return;
	/* End - BUG#M0000168 */
}

/* ----------
 * plisql_adddatum			Add a variable, record or row
 *					to the compiler's datum list.
 * ----------
 */
void
plisql_adddatum(PLiSQL_datum *newdatum)
{
	if (plisql_nDatums == datums_alloc)
	{
		datums_alloc *= 2;
		plisql_Datums = repalloc(plisql_Datums, sizeof(PLiSQL_datum *) * datums_alloc);
	}

	newdatum->dno = plisql_nDatums;
	plisql_Datums[plisql_nDatums++] = newdatum;
}

/* ----------
 * plisql_finish_datums	Copy completed datum info into function struct.
 * ----------
 */
void
plisql_finish_datums(PLiSQL_function *function)
{
	Size		copiable_size = 0;
	int			i;

	/* Begin - ReqID:SRS-SQL-PACKAGE */
	/*
	 * maybe compile a package body which function->ndatums not null
	 */
	if (function->item != NULL)
	{
		Assert(function->ndatums <= plisql_nDatums);

		if (function->ndatums == plisql_nDatums)
			return;
		if (function->ndatums != 0)
			function->datums = repalloc(function->datums,
									sizeof(PLiSQL_datum *) * plisql_nDatums);
		else
			function->datums = palloc(sizeof(PLiSQL_datum *) * plisql_nDatums);
		function->ndatums = plisql_nDatums;
	}
	else
	{
		function->ndatums = plisql_nDatums;
		function->datums = palloc(sizeof(PLiSQL_datum *) * plisql_nDatums);
	}
	/* End - ReqID:SRS-SQL-PACKAGE */
	for (i = 0; i < plisql_nDatums; i++)
	{
		function->datums[i] = plisql_Datums[i];

		/* This must agree with copy_plisql_datums on what is copiable */
		switch (function->datums[i]->dtype)
		{
			case PLISQL_DTYPE_VAR:
			case PLISQL_DTYPE_PROMISE:
				copiable_size += MAXALIGN(sizeof(PLiSQL_var));
				break;
			case PLISQL_DTYPE_REC:
				copiable_size += MAXALIGN(sizeof(PLiSQL_rec));
				break;
			/* Begin - ReqID:SRS-PLSQL-CURSOR */
			case PLISQL_DTYPE_ATTRIBUTE:
				copiable_size += MAXALIGN(sizeof(PLiSQL_attribute));
				break;
			/* End - ReqID:SRS-PLSQL-CURSOR */
			/* Begin - ReqID:SRS-PLSQL-RECORD */
			case PLISQL_DTYPE_TYPE:
				break;
			case PLISQL_DTYPE_ORA_RECORD:
				copiable_size += MAXALIGN(sizeof(PLiSQL_row));
				break;
			/* End - ReqID:SRS-PLSQL-RECORD */
			default:
				break;
		}
	}
	function->copiable_size = copiable_size;
}


/* ----------
 * plisql_add_initdatums		Make an array of the datum numbers of
 *					all the initializable datums created since the last call
 *					to this function.
 *
 * If varnos is NULL, we just forget any datum entries created since the
 * last call.
 *
 * This is used around a DECLARE section to create a list of the datums
 * that have to be initialized at block entry.  Note that datums can also
 * be created elsewhere than DECLARE, eg by a FOR-loop, but it is then
 * the responsibility of special-purpose code to initialize them.
 * ----------
 */
int
plisql_add_initdatums(int **varnos)
{
	int			i;
	int			n = 0;

	/*
	 * The set of dtypes recognized here must match what exec_stmt_block()
	 * cares about (re)initializing at block entry.
	 */
	for (i = datums_last; i < plisql_nDatums; i++)
	{
		switch (plisql_Datums[i]->dtype)
		{
			/* Begin - ReqID:SRS-PLSQL-CURSOR */
			case PLISQL_DTYPE_ATTRIBUTE:
			/* End - ReqID:SRS-PLSQL-CURSOR */
			case PLISQL_DTYPE_VAR:
			case PLISQL_DTYPE_REC:
			/* Begin - ReqID:SRS-PLSQL-RECORD */
			case PLISQL_DTYPE_TYPE:
			case PLISQL_DTYPE_ORA_RECORD:
			case PLISQL_DTYPE_ROW:
			case PLISQL_DTYPE_PACKAGE_DATUM:
			/* End - ReqID:SRS-PLSQL-RECORD */
				n++;
				break;
			default:
				break;
		}
	}

	if (varnos != NULL)
	{
		if (n > 0)
		{
			*varnos = (int *) palloc(sizeof(int) * n);

			n = 0;
			for (i = datums_last; i < plisql_nDatums; i++)
			{
				switch (plisql_Datums[i]->dtype)
				{
					/* Begin - ReqID:SRS-PLSQL-CURSOR */
					case PLISQL_DTYPE_ATTRIBUTE:
					/* End - ReqID:SRS-PLSQL-CURSOR */
					case PLISQL_DTYPE_VAR:
					case PLISQL_DTYPE_REC:
					/* Begin - ReqID:SRS-PLSQL-RECORD */
					case PLISQL_DTYPE_TYPE:
					case PLISQL_DTYPE_ORA_RECORD:
					case PLISQL_DTYPE_ROW:
					/* End - ReqID:SRS-PLSQL-RECORD */
						(*varnos)[n++] = plisql_Datums[i]->dno;

					default:
						break;
				}
			}
		}
		else
			*varnos = NULL;
	}

	datums_last = plisql_nDatums;
	return n;
}


/*
 * Compute the hashkey for a given function invocation
 *
 * The hashkey is returned into the caller-provided storage at *hashkey.
 */
static void
compute_function_hashkey(FunctionCallInfo fcinfo,
						 Form_pg_proc procStruct,
						 PLiSQL_func_hashkey *hashkey,
						 bool forValidator,
						 HeapTuple procTup) /* ReqID:SRS-SQL-PACKAGE */
{
	/* Make sure any unused bytes of the struct are zero */
	MemSet(hashkey, 0, sizeof(PLiSQL_func_hashkey));

	/* get function OID */
	hashkey->funcOid = fcinfo->flinfo->fn_oid;

	/* get call context */
	hashkey->isTrigger = CALLED_AS_TRIGGER(fcinfo);
	hashkey->isEventTrigger = CALLED_AS_EVENT_TRIGGER(fcinfo);

	/*
	 * If DML trigger, include trigger's OID in the hash, so that each trigger
	 * usage gets a different hash entry, allowing for e.g. different relation
	 * rowtypes or transition table names.  In validation mode we do not know
	 * what relation or transition table names are intended to be used, so we
	 * leave trigOid zero; the hash entry built in this case will never be
	 * used for any actual calls.
	 *
	 * We don't currently need to distinguish different event trigger usages
	 * in the same way, since the special parameter variables don't vary in
	 * type in that case.
	 */
	if (hashkey->isTrigger && !forValidator)
	{
		TriggerData *trigdata = (TriggerData *) fcinfo->context;

		hashkey->trigOid = trigdata->tg_trigger->tgoid;
	}

	/* get input collation, if known */
	hashkey->inputCollation = fcinfo->fncollation;

	if (procStruct->pronargs > 0)
	{
		/* get the argument types */
		/* Begin - ReqID:SRS-SQL-PACKAGE */
		Datum protypenames;
		bool	isNull;
		Datum	   *elems;
		int			nelems;
		char **argtypenames = NULL;
		int i;

		protypenames = SysCacheGetAttr(PROCOID, procTup,
									Anum_pg_proc_protypenames,
									&isNull);
		if (!isNull)
		{
			deconstruct_array(DatumGetArrayTypeP(protypenames),
								TEXTOID, -1, false, 'i',
								&elems, NULL, &nelems);
			argtypenames = (char **) palloc(sizeof(char *) * nelems);
			for (i = 0; i < nelems; i++)
				argtypenames[i] = TextDatumGetCString(elems[i]);
		}
		if (argtypenames != NULL)
		{
			for (i = 0; i < procStruct->pronargs; i++)
			{
				hashkey->argtypes[i] = procStruct->proargtypes.values[i];
				if (strcmp(argtypenames[i], "") != 0)
				{
					TypeName	*tname;
					PkgType *pkgtype;

					tname = (TypeName *) stringToNode(argtypenames[i]);

					pkgtype = LookupPkgTypeByTypename(tname->names, false);
					if (pkgtype == NULL)
					{
						/* Begin - ReqID:SRS-PLSQL-TYPE */
						Type		typtup;

						typtup = LookupOraTypeName(NULL, tname, NULL, true);
						if (typtup)
						{
							if (!((Form_pg_type) GETSTRUCT(typtup))->typisdefined)
							{
								pfree(tname);
								elog(ERROR, "package, relation or view does not exist");
							}
							hashkey->argtypes[i] = typeTypeId(typtup);
							ReleaseSysCache(typtup);
						}
						/* End - ReqID:SRS-PLSQL-TYPE */
					}
					else
					{
						hashkey->argtypes[i] = pkgtype->basetypid;
						pfree(pkgtype);
					}

					pfree(tname);
				}
			}
		}
		else
			memcpy(hashkey->argtypes, procStruct->proargtypes.values,
				procStruct->pronargs * sizeof(Oid));
		/* End - ReqID:SRS-SQL-PACKAGE */

		/* resolve any polymorphic argument types */
		plisql_resolve_polymorphic_argtypes(procStruct->pronargs,
											 hashkey->argtypes,
											 NULL,
											 fcinfo->flinfo->fn_expr,
											 forValidator,
											 NameStr(procStruct->proname));
	}
}

/*
 * This is the same as the standard resolve_polymorphic_argtypes() function,
 * but with a special case for validation: assume that polymorphic arguments
 * are integer, integer-array or integer-range.  Also, we go ahead and report
 * the error if we can't resolve the types.
 */
void
plisql_resolve_polymorphic_argtypes(int numargs,
									 Oid *argtypes, char *argmodes,
									 Node *call_expr, bool forValidator,
									 const char *proname)
{
	int			i;

	if (!forValidator)
	{
		/* normal case, pass to standard routine */
		if (!resolve_polymorphic_argtypes(numargs, argtypes, argmodes,
										  call_expr))
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("could not determine actual argument "
							"type for polymorphic function \"%s\"",
							proname)));
	}
	else
	{
		/* special validation case */
		for (i = 0; i < numargs; i++)
		{
			switch (argtypes[i])
			{
				case ANYELEMENTOID:
				case ANYNONARRAYOID:
				case ANYENUMOID:	/* XXX dubious */
				case ANYCOMPATIBLEOID:
				case ANYCOMPATIBLENONARRAYOID:
					argtypes[i] = INT4OID;
					break;
				case ANYARRAYOID:
				case ANYCOMPATIBLEARRAYOID:
					argtypes[i] = INT4ARRAYOID;
					break;
				case ANYRANGEOID:
				case ANYCOMPATIBLERANGEOID:
					argtypes[i] = INT4RANGEOID;
					break;
				case ANYMULTIRANGEOID:
					argtypes[i] = INT4MULTIRANGEOID;
					break;
				default:
					break;
			}
		}
	}
}

/*
 * delete_function - clean up as much as possible of a stale function cache
 *
 * We can't release the PLiSQL_function struct itself, because of the
 * possibility that there are fn_extra pointers to it.  We can release
 * the subsidiary storage, but only if there are no active evaluations
 * in progress.  Otherwise we'll just leak that storage.  Since the
 * case would only occur if a pg_proc update is detected during a nested
 * recursive call on the function, a leak seems acceptable.
 *
 * Note that this can be called more than once if there are multiple fn_extra
 * pointers to the same function cache.  Hence be careful not to do things
 * twice.
 */
void
delete_function(PLiSQL_function *func)
{
	/* remove function from hash table (might be done already) */
	plisql_HashTableDelete(func);

	/* release the function's storage if safe and not done already */
	if (func->use_count == 0)
		plisql_free_function_memory(func, 0, 0); /* SRS-PLSQL-SUBPROC */
}

/* exported so we can call it from _PG_init() */
void
plisql_HashTableInit(void)
{
	HASHCTL		ctl;

	/* don't allow double-initialization */
	Assert(plisql_HashTable == NULL);

	ctl.keysize = sizeof(PLiSQL_func_hashkey);
	ctl.entrysize = sizeof(plisql_HashEnt);
	plisql_HashTable = hash_create("PLiSQL function hash",
									FUNCS_PER_USER,
									&ctl,
									HASH_ELEM | HASH_BLOBS);
}


static PLiSQL_function *
plisql_HashTableLookup(PLiSQL_func_hashkey *func_key)
{
	plisql_HashEnt *hentry;

	hentry = (plisql_HashEnt *) hash_search(plisql_HashTable,
											 (void *) func_key,
											 HASH_FIND,
											 NULL);
	if (hentry)
		return hentry->function;
	else
		return NULL;
}

static void
plisql_HashTableInsert(PLiSQL_function *function,
						PLiSQL_func_hashkey *func_key)
{
	plisql_HashEnt *hentry;
	bool		found;

	hentry = (plisql_HashEnt *) hash_search(plisql_HashTable,
											 (void *) func_key,
											 HASH_ENTER,
											 &found);
	if (found)
		elog(WARNING, "trying to insert a function that already exists");

	hentry->function = function;
	/* prepare back link from function to hashtable key */
	function->fn_hashkey = &hentry->key;
}

static void
plisql_HashTableDelete(PLiSQL_function *function)
{
	plisql_HashEnt *hentry;

	/* do nothing if not in table */
	if (function->fn_hashkey == NULL)
		return;

	hentry = (plisql_HashEnt *) hash_search(plisql_HashTable,
											 (void *) function->fn_hashkey,
											 HASH_REMOVE,
											 NULL);
	if (hentry == NULL)
		elog(WARNING, "trying to delete function that does not exist");

	/* remove back link, which no longer points to allocated storage */
	function->fn_hashkey = NULL;
}

/* Begin - ReqID:SRS-SQL-LIBPQOUT */
/* ----------
 * plisql_compile_inline_internal	Get all the placeholers of an anonymous code block.
 * All the placeholers are stored in CurrentOraParamNode array.
 * Only compile the function, but not store the compiled function.
 * ----------
 */
void
plisql_compile_inline_internal(char *proc_source)
{
	char	   *func_name = "inline_code_block";
	PLiSQL_function *function;
	ErrorContextCallback plerrcontext;
	int			parse_rc;
	MemoryContext func_cxt;

	/*
	 * Setup the scanner input and error info.  We assume that this function
	 * cannot be invoked recursively, so there's no need to save and restore
	 * the static variables used here.
	 */
	plisql_scanner_init(proc_source);

	plisql_error_funcname = func_name;

	/*
	 * Setup error traceback support for ereport()
	 */
	plerrcontext.callback = plisql_compile_error_callback;
	plerrcontext.arg = proc_source;
	plerrcontext.previous = error_context_stack;
	error_context_stack = &plerrcontext;

	/* Do extra syntax checking if check_function_bodies is on */
	plisql_check_syntax = check_function_bodies;

	/* Function struct does not live past current statement */
	function = (PLiSQL_function *) palloc0(sizeof(PLiSQL_function));

	plisql_curr_compile = function;

	/*
	 * All the rest of the compile-time storage (e.g. parse tree) is kept in
	 * its own memory context, so it can be reclaimed easily.
	 */
	func_cxt = AllocSetContextCreate(CurrentMemoryContext,
									 "PL/iSQL inline code context",
									 ALLOCSET_DEFAULT_SIZES);
	plisql_compile_tmp_cxt = MemoryContextSwitchTo(func_cxt);

	function->fn_signature = pstrdup(func_name);
	function->fn_is_trigger = PLISQL_NOT_TRIGGER;
	function->fn_input_collation = InvalidOid;
	function->fn_cxt = func_cxt;
	function->out_param_varno = -1; /* set up for no OUT param */
	function->resolve_option = plisql_variable_conflict;
	function->print_strict_params = plisql_print_strict_params;

	/*
	 * don't do extra validation for inline code as we don't want to add spam
	 * at runtime
	 */
	function->extra_warnings = 0;
	function->extra_errors = 0;

	function->nstatements = 0;
	function->requires_procedure_resowner = false;
	/* Begin - ReqID:SRS-SQL-FUNCOUT */
	function->fn_ret_vardno = -1;
	function->fn_no_return = false;
	/* End - ReqID:SRS-SQL-FUNCOUT */

	/* Begin - ReqID:SRS-PLSQL-GOTO */
	plisql_start_label_compile(func_name);
	/* End - ReqID:SRS-PLSQL-GOTO */
	plisql_ns_init();
	plisql_ns_push(func_name, PLISQL_LABEL_BLOCK);
	plisql_DumpExecTree = false;
	cur_compile_func_level = 0;
	plisql_start_datums();
	/* Begin - SRS-PLSQL-SUBPROC */
	plisql_start_subproc_func();
	/* End - SRS-PLSQL-SUBPROC */
	/* Begin - ReqID:SRS-SQL-PACKAGE */
	plisql_compile_packageitem = NULL;
	Assert(plisql_curr_global_proper_level == 0);
	plisql_saved_compile[cur_compile_func_level] = function;
	/* End - ReqID:SRS-SQL-PACKAGE */

	/* Set up as though in a function returning VOID */
	function->fn_rettype = VOIDOID;
	function->fn_retset = false;
	function->fn_retistuple = false;
	function->fn_retisdomain = false;
	function->fn_prokind = PROKIND_ANONYMOUS_BLOCK;	/* anonymous block */
	/* a bit of hardwired knowledge about type VOID here */
	function->fn_retbyval = true;
	function->fn_rettyplen = sizeof(int32);
	/* Begin - ReqID:SRS-PLSQL-RECORD */
	function->fn_hasudtype = false;
	/* End - ReqID:SRS-PLSQL-RECORD */
	/* Begin - ReqID:SRS-PLSQL-DSQL */
	function->paramCheck = NULL;
	function->oraCallParamCheck = -1;
	/* End - ReqID:SRS-PLSQL-DSQL */

	/*
	 * Remember if function is STABLE/IMMUTABLE.  XXX would it be better to
	 * set this true inside a read-only transaction?  Not clear.
	 */
	function->fn_readonly = false;

	/* Begin - ReqID:SRS-SQL-ANONYOUT */
	/*
	 * Create the variables for the procedure's parameters.
	 */
	function->fn_nargs = 0;

	/*
	 * If we only parse a anonymous, set the prokind to PROKIND_ANONYMOUS_BLOCK_ONLY_PARSE,
	 * the normal anonymous prokind is PROKIND_ANONYMOUS_BLOCK,
	 */
	function->fn_prokind = PROKIND_ANONYMOUS_BLOCK_ONLY_PARSE;

	/*
	 * Now parse the function's text
	 */
	parse_rc = plisql_yyparse();
	if (parse_rc != 0)
		elog(ERROR, "plisql parser returned %d", parse_rc);
	function->action = plisql_parse_result;

	/* Begin - ReqID:SRS-PLSQL-GOTO */
	plisql_finish_label_compile();
	/* End - ReqID:SRS-PLSQL-GOTO */

	/*
	 * Complete the function's info
	 */
	plisql_finish_datums(function);

	/* Begin - SRS-PLSQL-SUBPROC */
	plisql_finish_subproc_func(function);
	/* End - SRS-PLSQL-SUBPROC */

	/* Begin - ReqID:SRS-SQL-PACKAGE */
	plisql_check_subproc_define(function);

	/*
	 * after plisql_check_subproc_define for nice error message
	 */
	plisql_scanner_finish();
	/* End - ReqID:SRS-SQL-PACKAGE */

	/*
	 * Pop the error context stack
	 */
	error_context_stack = plerrcontext.previous;
	plisql_error_funcname = NULL;

	plisql_check_syntax = false;

	MemoryContextSwitchTo(plisql_compile_tmp_cxt);
	plisql_compile_tmp_cxt = NULL;

	delete_function(function);
}

/*
 * dynamic_build_func_vars  Build the variables dynamiclly.
 * If find a ORAPARAM token, and the function is PROKIND_ANONYMOUS_BLOCK_ONLY_PARSE,
 * Build the variables dynamiclly.
 * In the example, :x and :y are ORAPARAM,  build two variables, $1 and $2.
 *
 * select * from get_parameter_descr('
 * begin
 *	 :x := id;
 *	 :y := 1;
 * end;');
 */
void
dynamic_build_func_vars(PLiSQL_function **function)
{
	char		buf[32];
	Oid 		argtypeid = INT4OID;
	PLiSQL_type *argdtype;
	PLiSQL_variable *argvariable;
	int 		argitemtype;
	int			fn_nargs = (*function)->fn_nargs;

	/* Create $n name for variable */
	snprintf(buf, sizeof(buf), "$%d", fn_nargs + 1);

	/* Create datatype info */
	argdtype = plisql_build_datatype(argtypeid,
								-1,
								(*function)->fn_input_collation,
								NULL);

	/* Build variable and add to datum list */
	argvariable = plisql_build_variable(buf, 0,
										argdtype, false);
	(*function)->fn_argvarnos[fn_nargs++] = argvariable->dno;
	(*function)->fn_nargs = fn_nargs;

	argitemtype = PLISQL_NSTYPE_VAR;

	/* Add to namespace under the $n name */
	add_parameter_name(argitemtype, argvariable->dno, buf);
	/* Begin - ReqID:SRS-PLSQL-CURSOR */
	if (ISORACURSOROID(argdtype->typoid))
	{
		set_plisql_cursorvar_nsitem_typoid(argvariable, true);
	}
	/* End - ReqID:SRS-PLSQL-CURSOR */
}
/* End - ReqID:SRS-SQL-LIBPQOUT */

/* Begin - ReqID:SRS-PLSQL-TYPE */
/*
 * plisql_free_function
 * If we have a compiled function, and it is still valid,
 * remove the function from hashtable.
 */
void
plisql_free_function(Oid funcOid)
{
	HeapTuple	procTup;
	Form_pg_proc proc;
	PLiSQL_function *function;
	PLiSQL_func_hashkey hashkey;
	char		functyptype;
	LOCAL_FCINFO(fake_fcinfo, 0);
	FmgrInfo	flinfo;
	TriggerData trigdata;
	EventTriggerData etrigdata;
	bool		is_dml_trigger = false;
	bool		is_event_trigger = false;

	/*
	 * Lookup the pg_proc tuple by Oid; we'll need it in any case
	 */
	procTup = SearchSysCache1(PROCOID, ObjectIdGetDatum(funcOid));
	if (!HeapTupleIsValid(procTup))
		elog(ERROR, "cache lookup failed for function %u", funcOid);
	proc = (Form_pg_proc) GETSTRUCT(procTup);

	functyptype = get_typtype(proc->prorettype);

	/* Disallow pseudotype result */
	/* except for TRIGGER, EVTTRIGGER, RECORD, VOID, or polymorphic */
	if (functyptype == TYPTYPE_PSEUDO)
	{
		if (proc->prorettype == TRIGGEROID)
			is_dml_trigger = true;
		else if (proc->prorettype == EVENT_TRIGGEROID)
			is_event_trigger = true;
		else if (proc->prorettype != RECORDOID &&
				 /* Begin - ReqID:SRS-PLSQL-RECORD */
				 proc->prorettype != ORARECORDOID &&
				 /* End - ReqID:SRS-PLSQL-RECORD */
				 proc->prorettype != VOIDOID &&
				 !IsPolymorphicType(proc->prorettype))
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("PL/pgSQL functions cannot return type %s",
							format_type_be(proc->prorettype))));
	}

	/*
	 * Set up a fake fcinfo with just enough info to satisfy
	 * compute_function_hashkey().
	 */
	MemSet(fake_fcinfo, 0, SizeForFunctionCallInfo(0));
	MemSet(&flinfo, 0, sizeof(flinfo));
	fake_fcinfo->flinfo = &flinfo;
	flinfo.fn_oid = funcOid;
	flinfo.fn_mcxt = CurrentMemoryContext;
	if (is_dml_trigger)
	{
		MemSet(&trigdata, 0, sizeof(trigdata));
		trigdata.type = T_TriggerData;
		fake_fcinfo->context = (Node *) &trigdata;
	}
	else if (is_event_trigger)
	{
		MemSet(&etrigdata, 0, sizeof(etrigdata));
		etrigdata.type = T_EventTriggerData;
		fake_fcinfo->context = (Node *) &etrigdata;
	}

	/* Compute hashkey using function signature and actual arg types */
	compute_function_hashkey(fake_fcinfo, proc, &hashkey, true,
							procTup);

	/* And do the lookup */
	function = plisql_HashTableLookup(&hashkey);

	if (function)
	{
		/* We have a compiled function, and it is still valid, remove the function from hashtable */
		if (function->fn_xmin == HeapTupleHeaderGetRawXmin(procTup->t_data) &&
			ItemPointerEquals(&function->fn_tid, &procTup->t_self))
		{
			/*
			 * Remove the function from hashtable.
			 */
			delete_function(function);
		}
	}

	ReleaseSysCache(procTup);
}

/*
 * plisql_add_type_referenced_objects
 * Check %TYPE or %ROWTYPE type which may reference a table, view, package
 */
static void
plisql_add_type_referenced_objects(TypeName *typeName)
{
	PkgType *pkgtype;

	pkgtype = LookupPkgTypeByTypename(typeName->names, false);
	if (pkgtype != NULL)
	{
		ObjectAddress *refobj;

		refobj = (ObjectAddress *)palloc(sizeof(ObjectAddress));
		refobj->classId = PackageRelationId;
		refobj->objectId = pkgtype->pkgoid;
		refobj->objectSubId = InvalidAttrNumber;

		plisql_referenced_objects = lappend(plisql_referenced_objects, refobj);

		pfree(pkgtype);
	}
	else if (typeName != NULL && typeName->pct_type)
	{
		/* Handle %TYPE reference to type of an existing field */
		RangeVar   *rel = makeRangeVar(NULL, NULL, typeName->location);
		char	   *field = NULL;
		Oid 		relid;
		AttrNumber	attnum;
		ObjectAddress *refobj;

		/* deconstruct the name list */
		switch (list_length(typeName->names))
		{
			case 1:
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("improper %%TYPE reference (too few dotted names): %s",
								NameListToString(typeName->names))));
				break;
			case 2:
				rel->relname = strVal(linitial(typeName->names));
				field = strVal(lsecond(typeName->names));
				break;
			case 3:
				rel->schemaname = strVal(linitial(typeName->names));
				rel->relname = strVal(lsecond(typeName->names));
				field = strVal(lthird(typeName->names));
				break;
			case 4:
				rel->catalogname = strVal(linitial(typeName->names));
				rel->schemaname = strVal(lsecond(typeName->names));
				rel->relname = strVal(lthird(typeName->names));
				field = strVal(lfourth(typeName->names));
				break;
			default:
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("improper %%TYPE reference (too many dotted names): %s",
								NameListToString(typeName->names))));
				break;
		}

		/*
		 * Look up the field.
		 *
		 * XXX: As no lock is taken here, this might fail in the presence of
		 * concurrent DDL.	But taking a lock would carry a performance
		 * penalty and would also require a permissions check.
		 */
		relid = RangeVarGetRelid(rel, NoLock, false);
		attnum = get_attnum(relid, field);
		if (attnum == InvalidAttrNumber)
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_COLUMN),
					 errmsg("column \"%s\" of relation \"%s\" does not exist",
							field, rel->relname)));

		refobj = (ObjectAddress *)palloc(sizeof(ObjectAddress));
		refobj->classId = RelationRelationId;
		refobj->objectId = relid;
		refobj->objectSubId = attnum;
		plisql_referenced_objects = lappend(plisql_referenced_objects, (void *)refobj);

	}
	else if (typeName != NULL && typeName->row_type)
	{
		char		*schema_name = NULL;
		char		*relname = NULL;
		RangeVar   *rel;
		Oid 		relid;

		DeconstructQualifiedName(typeName->names, &schema_name, &relname);
		rel = makeRangeVar(schema_name, relname, typeName->location);

		relid = RangeVarGetRelid(rel, NoLock, false);
		if (OidIsValid(relid))
		{
			ObjectAddress *refobj;

			refobj = (ObjectAddress *)palloc(sizeof(ObjectAddress));
			refobj->classId = RelationRelationId;
			refobj->objectId = relid;
			refobj->objectSubId = InvalidAttrNumber;
			plisql_referenced_objects = lappend(plisql_referenced_objects, (void *)refobj);
		}
	}

	pfree(typeName);
}

/* End - ReqID:SRS-PLSQL-TYPE */


/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
/*
 * get errcode match errormsg
 */
const char *
plisql_recognize_err_message_int(int oraerrcode)
{
	char *errmsg = NULL;
	int i;

	if (oraerrcode == PLISQL_UDT_EXCEPTION_DEFAULT_SQLCODE)
		return PLISQL_UDT_EXCEPTION_DEFAULT_ERRMSG;
	else if (oraerrcode == 0)
		return "IVY-0000: normal, successful completion";

	plisql_get_exception_pgerrcode(CurrentMemoryContext,
												oraerrcode,
												&errmsg);
	if (errmsg != NULL)
		return errmsg;

	/* doesn't found, should map pg message */
	for (i = 0; exception_label_map[i].label != NULL; i++)
	{
		if (exception_label_map[i].sqlerrstate == oraerrcode)
			return exception_label_map[i].label;
	}

	/* not map, we report oracle error msg */
	if (oraerrcode == 100)
		return pstrdup("IVY-01403: no data found");
	else if (oraerrcode > 0)
	{
		StringInfoData data;

		initStringInfo(&data);
		appendStringInfo(&data, "-%d:non-IVORYSQL exception", oraerrcode);

		errmsg = data.data;
	}
	/* -20999 ~ -20000 */
	else if (oraerrcode <= -20000 && oraerrcode >= -20999)
	{
		int sqlcode;
		char *sqlerrm;

		/*
		 * raise_application_error may be has invoke, so we
		 * get current statck error message
		 */
		sqlcode = get_current_sqlcode_sqlerrm(&sqlerrm);
		if (sqlcode == oraerrcode && sqlerrm != NULL)
			errmsg = sqlerrm;
		else
		{
			StringInfoData data;

			initStringInfo(&data);
			appendStringInfo(&data, "IVY%d:",
					oraerrcode);

			errmsg = data.data;
		}
	}
	else
	{
		StringInfoData data;

		initStringInfo(&data);
		appendStringInfo(&data, "IVY%d: Message %d not found"
				";product=RDBMS;facility=IVY",
				oraerrcode,
				0 - oraerrcode);

		errmsg = data.data;
	}

	return errmsg;
}
/* End - ReqID:SRS-PLSQL-EXCEPTION */

/* Begin - ReqID:SRS-PLSQL-SUBTYPE */
/*
 * To dumplicate a basetype so that we can independently configure
 * the typmod and NOT NULL constraints.
 * 
 * Return a PLiSQL_datum.
 */
PLiSQL_datum*
plisql_copy_base_datatype(PLiSQL_type *in, char *subtypnme, int lineno)
{
	PLiSQL_datum *out;

	switch (in->ttype)
	{
		case PLISQL_TTYPE_SCALAR:
		case PLISQL_TTYPE_PSEUDO:
			{
				PLiSQL_type *basetype;
				PLiSQL_datum_udt_type *datum_type;

				basetype = (PLiSQL_type *)palloc0(sizeof(PLiSQL_type));
				memcpy(basetype, in, sizeof(PLiSQL_type));

				datum_type = palloc0(sizeof(PLiSQL_datum_udt_type));
				datum_type->dtype = PLISQL_DTYPE_TYPE;
				datum_type->udt_type = basetype;
				datum_type->pkgoid = (plisql_compile_packageitem == NULL ? InvalidOid : plisql_compile_packageitem->source.fn_oid);

				out = (PLiSQL_datum *)datum_type;
				break;
			}
		case PLISQL_TTYPE_REC:
			{
				PLiSQL_type *basetype;
				PLiSQL_rec *out_rec;

				basetype = (PLiSQL_type *)palloc0(sizeof(PLiSQL_type));
				memcpy(basetype, in, sizeof(PLiSQL_type));

				out_rec = (PLiSQL_rec *)palloc0(sizeof(PLiSQL_rec));
				out_rec->dtype = PLISQL_DTYPE_REC;
				out_rec->refname = pstrdup(subtypnme);
				out_rec->lineno = lineno;
				out_rec->datatype = basetype;
				out_rec->rectypeid = in->typoid;
				out_rec->firstfield = -1;
				out_rec->erh = NULL;
				out_rec->pkgoid = (plisql_compile_packageitem == NULL ? InvalidOid : plisql_compile_packageitem->source.fn_oid);
				plisql_expand_rec_field(out_rec);

				out = (PLiSQL_datum *)out_rec;
				break;
			}
		case PLISQL_TTYPE_EXPLICIT_CURSOR:
			{
				PLiSQL_explicit_cursor_type *out_cursor_type;
				PLiSQL_datum_udt_type *datum_type;

				out_cursor_type = (PLiSQL_explicit_cursor_type *)palloc0(sizeof(PLiSQL_explicit_cursor_type));
				memcpy(out_cursor_type, in, sizeof(PLiSQL_explicit_cursor_type));

				datum_type = palloc0(sizeof(PLiSQL_datum_udt_type));
				datum_type->dtype = PLISQL_DTYPE_TYPE;
				datum_type->udt_type = (PLiSQL_type *) out_cursor_type;
				datum_type->pkgoid = (plisql_compile_packageitem == NULL ? InvalidOid : plisql_compile_packageitem->source.fn_oid);

				out = (PLiSQL_datum *)datum_type;
				break;
			}
		case PLISQL_TTYPE_UDT_REF_CURSOR:
			{
				PLiSQL_ref_cursor_type *out_cursor_type;
				PLiSQL_datum_udt_type *datum_type;

				out_cursor_type = (PLiSQL_ref_cursor_type *) palloc0(sizeof(PLiSQL_ref_cursor_type));
				memcpy(out_cursor_type, in, sizeof(PLiSQL_ref_cursor_type));

				datum_type = palloc0(sizeof(PLiSQL_datum_udt_type));
				datum_type->dtype = PLISQL_DTYPE_TYPE;
				datum_type->udt_type = (PLiSQL_type *) out_cursor_type;
				datum_type->pkgoid = (plisql_compile_packageitem == NULL ? InvalidOid : plisql_compile_packageitem->source.fn_oid);

				out = (PLiSQL_datum *)datum_type;
				break;
			}
		case PLISQL_TTYPE_ORARECORD_TYPE:
			{
				PLiSQL_ora_record_type *out_ora_rec;
				PLiSQL_datum_udt_type *datum_type;

				out_ora_rec = (PLiSQL_ora_record_type *) palloc0(sizeof(PLiSQL_ora_record_type));
				memcpy(out_ora_rec, in, sizeof(PLiSQL_ora_record_type));

				datum_type = palloc0(sizeof(PLiSQL_datum_udt_type));
				datum_type->dtype = PLISQL_DTYPE_TYPE;
				datum_type->udt_type = (PLiSQL_type *) out_ora_rec;
				datum_type->pkgoid = (plisql_compile_packageitem == NULL ? InvalidOid : plisql_compile_packageitem->source.fn_oid);

				out = (PLiSQL_datum *)datum_type;
				break;
			}
		case PLISQL_TTYPE_SUBTYPE:
			{
				PLiSQL_subtype_type *out_sub_type;
				PLiSQL_datum_udt_type *datum_type;

				out_sub_type = (PLiSQL_subtype_type *) palloc0(sizeof(PLiSQL_subtype_type));
				memcpy(out_sub_type, in, sizeof(PLiSQL_subtype_type));

				datum_type = palloc0(sizeof(PLiSQL_datum_udt_type));
				datum_type->dtype = PLISQL_DTYPE_TYPE;
				datum_type->udt_type = (PLiSQL_type *) out_sub_type;
				datum_type->pkgoid = (plisql_compile_packageitem == NULL ? InvalidOid : plisql_compile_packageitem->source.fn_oid);

				out = (PLiSQL_datum *)datum_type;
				break;
			}		
		default:
			elog(ERROR, "unrecognized ttype: %d", in->ttype);
			break;
	}

	return out;
}
/* End - ReqID:SRS-PLSQL-SUBTYPE */

/* Begin - ReqID:SRS-PLSQL-Collection */
/*
 *  transform collection subscript
 */
static Node *
plisql_transform_indirection(ParseState *pstate,
				Node *ind_arg,
				const char *targetName,
				List *indirection,
				Node *rhs,
				int location)
{
	Node	   *last_srf = pstate->p_last_srf;
	Node	   *result = ind_arg;
	List	   *subscripts = NIL;
	Oid			arg_type = exprType(result);
	ListCell   *i;
	PLiSQL_type *datatype = NULL;

	if (!IS_COLLECTOID(arg_type))
		return NULL;

	foreach(i, indirection)
	{
		Node	   *n = lfirst(i);

		if (IsA(n, A_Indices))
			subscripts = lappend(subscripts, n);
		else if (IsA(n, A_Star))
		{
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("row expansion via \"*\" is not supported here"),
					 parser_errposition(pstate, location)));
		}
		else
		{
			Node	   *newresult = NULL;

			Assert(IsA(n, String));

			/* process subscripts before this field selection */
			if (subscripts)
				result = (Node *) transformCollectionSubscripts(pstate,
															   result,
															   targetName,
															   exprType(result),
															   InvalidOid,
															   exprTypmod(result),
															   subscripts,
															   &datatype,
															   rhs,
															   location);
			subscripts = NIL;

			/*prcoess record direction */
			if (datatype->ttype == PLISQL_TTYPE_REC ||
				datatype->ttype == PLISQL_TTYPE_EXPLICIT_CURSOR)
			{
				TupleDesc 	tupdesc;
				int			i;

				tupdesc = plisql_get_rowtype_tupledesc(datatype);

				for(i = 0; i < tupdesc->natts; i++)
				{
					/* skip dropped column in tuple */
					if (tupdesc->attrs[i].attisdropped)
						continue;

					if (strcmp(strVal(n), NameStr(tupdesc->attrs[i].attname)) == 0)
					{
						/* Success, so generate a FieldSelect expression */
						FieldSelect *fselect = makeNode(FieldSelect);

						fselect->arg = (Expr *)result;
						fselect->fieldnum = i + 1;
						fselect->resulttype = tupdesc->attrs[i].atttypid;
						fselect->resulttypmod = tupdesc->attrs[i].atttypmod;

						fselect->resultcollid = -1;
						newresult = (Node *)fselect;
						break;
					}
				}
				ReleaseTupleDesc(tupdesc);
			}
			else if (datatype->ttype == PLISQL_TTYPE_ORARECORD_TYPE)
			{
				PLiSQL_ora_record_type	*rec_type = (PLiSQL_ora_record_type *)datatype;
				ListCell			*lc;
				int					 i = 0;

				foreach(lc, rec_type->args)
				{
					PLiSQL_ora_record_argitem	*argitem = (PLiSQL_ora_record_argitem*)lfirst(lc);

					 if (strcmp(strVal(n), argitem->argname) == 0)
					 {
					 	/* Success, so generate a FieldSelect expression */
						FieldSelect *fselect = makeNode(FieldSelect);

						fselect->arg = (Expr *)result;
						/*
						 * Because when build a record tupdesc, we add a column as
						 * the first column of redcord that is record datatype,
						 * so fselect->fieldnum is set to i + 2
						 */
						fselect->fieldnum = i + 2;
						fselect->resulttype = argitem->type->typoid;
						fselect->resulttypmod = argitem->type->atttypmod;
						/* save attribute's collation for parse_collate.c */
						fselect->resultcollid = argitem->type->collation;
						newresult = (Node *)fselect;

						datatype = argitem->type;

						break;
					 }
					 i++;
				}

			}
			else
				newresult = ParseFuncOrColumn(pstate,
											  list_make1(n),
											  list_make1(result),
											  last_srf,
											  NULL,
											  false,
											  location);

			if (newresult == NULL)
				ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_COLUMN),
					 errmsg("column \"%s\" not found in data type %s",
							strVal(n), datatype->typname),
					 parser_errposition(pstate, location)));

			result = newresult;
		}
	}

	/* the collection only has subscripts. process trailing subscripts */
	if (subscripts)
	{
		result = (Node *) transformCollectionSubscripts(pstate,
													   result,
													   targetName,
													   exprType(result),
													   InvalidOid,
													   exprTypmod(result),
													   subscripts,
													   &datatype,
													   rhs,
													   location);
	}

	return result;
}

/*
 *  transform collection subscirpt
 */
static SubscriptingRef *
transformCollectionSubscripts(ParseState *pstate,
							 Node *containerBase,
							 const char *targetName,
							 Oid containerType,
							 Oid elementType,
							 int32 containerTypMod,
							 List *indirection,
							 PLiSQL_type **datatype,
							 Node *rhs,
							 int location)
{
	PLiSQL_type 		*type_info;
	ListCell			*idx;
	List				*indexpr = NIL;
	SubscriptingRef 	*sbsref;
	/* Begin - ReqID:SRS-PLSQL-FORALL */
	A_Indices			*last_index = NULL;
	/* End - ReqID:SRS-PLSQL-FORALL */

	if (datatype != NULL && *datatype != NULL)
		type_info = *datatype;
	else if (IsA(containerBase, Param))
	{
		Param *pm = (Param *) containerBase;
		type_info = pm->type;
	}
	else
		ereport(ERROR,
			(errcode(ERRCODE_VALUE_ERROR),
			errmsg("can't get the collection type infomation")));

	/* Begin - ReqID:SRS-PLSQL-FORALL */
	last_index = (A_Indices *) llast(indirection);
	/* End - ReqID:SRS-PLSQL-FORALL */

	/* Transform the subscript expressions.*/
	foreach(idx, indirection)
	{
		A_Indices	*ai = (A_Indices *) lfirst(idx);
		PLiSQL_ora_collection_type *collect_type;
		Node		*subexpr;
		Node		*new_expr;

		if (!IS_PLSQL_COLLECTYPE(type_info->typtype))
			ereport(ERROR,
			(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
			 errmsg("number of subscripts exceeds the maximum udt dimensions.")));

		collect_type = (PLiSQL_ora_collection_type *)type_info;

		if (ai->uidx)
		{
			/* Begin - ReqID:SRS-PLSQL-FORALL */
			bool	saved_is_collect_subscript = false;

			if (ai == last_index &&
				pstate->p_ref_hook_state != NULL &&
				((PLiSQL_expr *) pstate->p_ref_hook_state)->forallparams != NULL)
			{
				PLiSQL_expr 	*expr = (PLiSQL_expr *) pstate->p_ref_hook_state;

				saved_is_collect_subscript = expr->is_collect_subscript;
				expr->is_collect_subscript = true;
			}
			/* End - ReqID:SRS-PLSQL-FORALL */

			subexpr = transformExpr(pstate, ai->uidx, pstate->p_expr_kind);

			/* Begin - ReqID:SRS-PLSQL-FORALL */
			if (ai == last_index &&
				pstate->p_ref_hook_state != NULL &&
				((PLiSQL_expr *) pstate->p_ref_hook_state)->forallparams != NULL)
			{
				PLiSQL_expr 	*expr = (PLiSQL_expr *) pstate->p_ref_hook_state;

				expr->is_collect_subscript = saved_is_collect_subscript;
			}
			/* End - ReqID:SRS-PLSQL-FORALL */

			new_expr = coerce_to_target_type(pstate,
											subexpr, exprType(subexpr),
											collect_type->typkey->typoid,
											collect_type->typkey->atttypmod,
											COERCION_ASSIGNMENT,
											COERCE_IMPLICIT_CAST,
											-1);
			if (new_expr == NULL)
				ereport(ERROR,
							(errcode(ERRCODE_DATATYPE_MISMATCH),
							 errmsg("collection subscript data type mismatch"),
							 parser_errposition(pstate, exprLocation(ai->lidx))));

			if(IsA(new_expr, Const) && ((Const *)new_expr)->constisnull)
				ereport(ERROR,
				(errcode(ERRCODE_VALUE_ERROR),
				errmsg("collection type subscript must not be NULL")));

			/* If there's no cast path according to the parser, fall back to using
			 * an I/O coercion; this is semantically dubious but matches plpgsql's
			 * historical behavior.  We would need something of the sort for
			 * UNKNOWN literals in any case.
			 */
		}
		else
			new_expr = NULL;

		indexpr = lappend(indexpr, new_expr);
		type_info = collect_type->typdatatype;
	}

	/*
	 * Ready to build the SubscriptingRef node.
	 */
	sbsref = (SubscriptingRef *) makeNode(SubscriptingRef);
	sbsref->refcontainertype = containerType;
	sbsref->refelemtype = type_info->typoid;
	sbsref->refrestype = type_info->typoid;
	sbsref->reftypmod = type_info->atttypmod;

	/* refcollid will be set by parse_collate.c */
	sbsref->reflowerindexpr = NIL;
	sbsref->refexpr = (Expr *) containerBase;
	sbsref->refupperindexpr = indexpr;
	sbsref->refassgnexpr = NULL;

	*datatype = type_info;
	return sbsref;

}

PLiSQL_type *
get_row_field_type(PLiSQL_type *data_type, char *name)
{
	TupleDesc	tupdesc;
	int 		fno;
	PLiSQL_type *type = NULL;

	tupdesc = plisql_get_rowtype_tupledesc(data_type);

	fno = SPI_fnumber(tupdesc, name);
	if (fno <= 0)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_COLUMN),
				 errmsg("record  has no field \"%s\"", name)));

	type = plisql_build_datatype(tupdesc->attrs[fno - 1].atttypid,
								tupdesc->attrs[fno - 1].atttypmod,
								tupdesc->attrs[fno - 1].attcollation,
								makeTypeName(NameStr(tupdesc->attrs[fno - 1].attname)));
	ReleaseTupleDesc(tupdesc);

	return type;
}
/* End - ReqID:SRS-PLSQL-Collection */
