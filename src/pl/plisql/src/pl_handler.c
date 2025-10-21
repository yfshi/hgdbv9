/*-------------------------------------------------------------------------
 *
 * pl_handler.c		- Handler for the PL/iSQL
 *			  procedural language
 *
 * Portions Copyright (c) 2022, IvorySQL-2.0
 *
 *
 * IDENTIFICATION
 *	  src/pl/plisql/src/pl_handler.c
 *
 * add the file for requirement "SQL PARSER"
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/htup_details.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_type.h"
#include "funcapi.h"
#include "miscadmin.h"
#include "plisql.h"
/* Begin - SRS-PLSQL-SUBPROC */
#include "pl_subproc_function.h"
/* End - SRS-PLSQL-SUBPROC*/
/* Begin - ReqID:SRS-SQL-PACKAGE */
#include "pl_package.h"
/* End - ReqID:SRS-SQL-PACKAGE */
/* Begin - ReqID:SRS-PLSQL-CURSOR */
#include "pl_udt_type.h"
/* End - ReqID:SRS-PLSQL-CURSOR */
/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
#include "pl_udt_exception.h"
/* End - ReqID:SRS-PLSQL-EXCEPTION */
#include "storage/proc.h"	/* ReqID:SRS-PLSQL-AUTONOMOUSTRANSACTION */
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"
#include "utils/snapmgr.h"	/* ReqID:SRS-PLSQL-AUTONOMOUSTRANSACTION */
#include "utils/syscache.h"
#include "utils/varlena.h"
/* Begin - ReqID:SRS-SQL-ANONYOUT */
#include "parser/parse_param.h"
#include "utils/datum.h"
/* End - ReqID:SRS-SQL-ANONYOUT */
/* Begin - ReqID:SRS-SQL-PACKAGE */
#include "commands/packagecmds.h"
/* End - ReqID:SRS-SQL-PACKAGE */

#include "catalog/dependency.h"

static bool plisql_extra_checks_check_hook(char **newvalue, void **extra, GucSource source);
static void plisql_extra_warnings_assign_hook(const char *newvalue, void *extra);
static void plisql_extra_errors_assign_hook(const char *newvalue, void *extra);

PG_MODULE_MAGIC;

/* Custom GUC variable */
static const struct config_enum_entry variable_conflict_options[] = {
	{"error", PLISQL_RESOLVE_ERROR, false},
	{"use_variable", PLISQL_RESOLVE_VARIABLE, false},
	{"use_column", PLISQL_RESOLVE_COLUMN, false},
	{NULL, 0, false}
};

int			plisql_variable_conflict = PLISQL_RESOLVE_ERROR;

bool		plisql_print_strict_params = false;

bool		plisql_check_asserts = true;

char	   *plisql_extra_warnings_string = NULL;
char	   *plisql_extra_errors_string = NULL;
int			plisql_extra_warnings;
int			plisql_extra_errors;

/* Begin - ReqID:SRS-PLSQL-CURSOR */
bool		plisql_close_unused_cursor = true;
bool		plisql_cursor_name_exclude_oid = false;
/* End - ReqID:SRS-PLSQL-CURSOR */

/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
bool		plisql_sqlerrm_use_oracle = true;
bool		plisql_name_composite_type_var_assign_check = true;
bool		plisql_default_into_strict = true;
/* End - ReqID:SRS-PLSQL-EXCEPTION */



/* Hook for plugins */
PLiSQL_plugin **plisql_plugin_ptr = NULL;

/* ReqID:SRS-PLSQL-AUTONOMOUSTRANSACTION */
static AutoxIsoVarsHookType	prev_RegisterAutonxIsoVars_PlisqlHook = NULL;

static bool
plisql_extra_checks_check_hook(char **newvalue, void **extra, GucSource source)
{
	char	   *rawstring;
	List	   *elemlist;
	ListCell   *l;
	int			extrachecks = 0;
	int		   *myextra;

	if (pg_strcasecmp(*newvalue, "all") == 0)
		extrachecks = PLISQL_XCHECK_ALL;
	else if (pg_strcasecmp(*newvalue, "none") == 0)
		extrachecks = PLISQL_XCHECK_NONE;
	else
	{
		/* Need a modifiable copy of string */
		rawstring = pstrdup(*newvalue);

		/* Parse string into list of identifiers */
		if (!SplitIdentifierString(rawstring, ',', &elemlist))
		{
			/* syntax error in list */
			GUC_check_errdetail("List syntax is invalid.");
			pfree(rawstring);
			list_free(elemlist);
			return false;
		}

		foreach(l, elemlist)
		{
			char	   *tok = (char *) lfirst(l);

			if (pg_strcasecmp(tok, "shadowed_variables") == 0)
				extrachecks |= PLISQL_XCHECK_SHADOWVAR;
			else if (pg_strcasecmp(tok, "too_many_rows") == 0)
				extrachecks |= PLISQL_XCHECK_TOOMANYROWS;
			else if (pg_strcasecmp(tok, "strict_multi_assignment") == 0)
				extrachecks |= PLISQL_XCHECK_STRICTMULTIASSIGNMENT;
			else if (pg_strcasecmp(tok, "all") == 0 || pg_strcasecmp(tok, "none") == 0)
			{
				GUC_check_errdetail("Key word \"%s\" cannot be combined with other key words.", tok);
				pfree(rawstring);
				list_free(elemlist);
				return false;
			}
			else
			{
				GUC_check_errdetail("Unrecognized key word: \"%s\".", tok);
				pfree(rawstring);
				list_free(elemlist);
				return false;
			}
		}

		pfree(rawstring);
		list_free(elemlist);
	}

	myextra = (int *) malloc(sizeof(int));
	if (!myextra)
		return false;
	*myextra = extrachecks;
	*extra = (void *) myextra;

	return true;
}

static void
plisql_extra_warnings_assign_hook(const char *newvalue, void *extra)
{
	plisql_extra_warnings = *((int *) extra);
}

static void
plisql_extra_errors_assign_hook(const char *newvalue, void *extra)
{
	plisql_extra_errors = *((int *) extra);
}


/*
 * _PG_init()			- library load-time initialization
 *
 * DO NOT make this static nor change its name!
 */
void
_PG_init(void)
{
	/* Be sure we do initialization only once (should be redundant now) */
	static bool inited = false;

	if (inited)
		return;

	pg_bindtextdomain(TEXTDOMAIN);

	DefineCustomEnumVariable("plisql.variable_conflict",
							 gettext_noop("Sets handling of conflicts between PL/iSQL variable names and table column names."),
							 NULL,
							 &plisql_variable_conflict,
							 PLISQL_RESOLVE_ERROR,
							 variable_conflict_options,
							 PGC_SUSET, 0,
							 NULL, NULL, NULL);

	DefineCustomBoolVariable("plisql.print_strict_params",
							 gettext_noop("Print information about parameters in the DETAIL part of the error messages generated on INTO ... STRICT failures."),
							 NULL,
							 &plisql_print_strict_params,
							 false,
							 PGC_USERSET, 0,
							 NULL, NULL, NULL);

	DefineCustomBoolVariable("plisql.check_asserts",
							 gettext_noop("Perform checks given in ASSERT statements."),
							 NULL,
							 &plisql_check_asserts,
							 true,
							 PGC_USERSET, 0,
							 NULL, NULL, NULL);

	DefineCustomStringVariable("plisql.extra_warnings",
							   gettext_noop("List of programming constructs that should produce a warning."),
							   NULL,
							   &plisql_extra_warnings_string,
							   "none",
							   PGC_USERSET, GUC_LIST_INPUT,
							   plisql_extra_checks_check_hook,
							   plisql_extra_warnings_assign_hook,
							   NULL);

	DefineCustomStringVariable("plisql.extra_errors",
							   gettext_noop("List of programming constructs that should produce an error."),
							   NULL,
							   &plisql_extra_errors_string,
							   "none",
							   PGC_USERSET, GUC_LIST_INPUT,
							   plisql_extra_checks_check_hook,
							   plisql_extra_errors_assign_hook,
							   NULL);
	/* Begin - ReqID:SRS-PLSQL-CURSOR */
	DefineCustomBoolVariable("plisql.auto_close_unused_cursor",
							 gettext_noop("Wether close unused cursor after exec function."),
							 NULL,
							 &plisql_close_unused_cursor,
							 true,
							 PGC_USERSET, 0,
							 NULL, NULL, NULL);
	DefineCustomBoolVariable("plisql.plisql_cursor_name_exclude_oid",
							 gettext_noop("Wether cursor default value exclude its oid."),
							 NULL,
							 &plisql_cursor_name_exclude_oid,
							 false,
							 PGC_USERSET, 0,
							 NULL, NULL, NULL);
	/* End - ReqID:SRS-PLSQL-CURSOR */
	/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
	DefineCustomBoolVariable("plisql.plisql_sqlerrm_use_oracle",
							 gettext_noop("Wether sqlerrm use message like oracle."),
							 NULL,
							 &plisql_sqlerrm_use_oracle,
							 true,
							 PGC_USERSET, 0,
							 NULL, NULL, NULL);
	DefineCustomBoolVariable("plisql.plisql_name_composite_type_var_assign_check",
							 gettext_noop("Wether allow variables of composite types to be checked within variables."),
							 NULL,
							 &plisql_name_composite_type_var_assign_check,
							 true,
							 PGC_USERSET, 0,
							 NULL, NULL, NULL);
	DefineCustomBoolVariable("plisql.plisql_default_into_strict",
							 gettext_noop("Default into followed strict."),
							 NULL,
							 &plisql_default_into_strict,
							 true,
							 PGC_USERSET, 0,
							 NULL, NULL, NULL);
	/* End - ReqID:SRS-PLSQL-EXCEPTION */
	EmitWarningsOnPlaceholders("plisql");

	plisql_HashTableInit();
	RegisterXactCallback(plisql_xact_cb, NULL);
	RegisterSubXactCallback(plisql_subxact_cb, NULL);

	/* Begin - SRS-PLSQL-SUBPROC */
	plisql_register_internal_func();
	/* End - SRS-PLSQL-SUBPROC */

	/* Begin - ReqID:SRS-PLSQL-CURSOR */
	plisql_register_orarefcursor_func();
	/* End - ReqID:SRS-PLSQL-CURSOR */

	/* Set up a rendezvous point with optional instrumentation plugin */
	plisql_plugin_ptr = (PLiSQL_plugin **) find_rendezvous_variable("PLiSQL_plugin");

	/* Begin - ReqID:SRS-PLSQL-AUTONOMOUSTRANSACTION */
	prev_RegisterAutonxIsoVars_PlisqlHook = RegisterAutonxIsoVars_PlisqlHook;
	RegisterAutonxIsoVars_PlisqlHook = RegisterAutonxIsoVars_plisql_hook;
	/* End - ReqID:SRS-PLSQL-AUTONOMOUSTRANSACTION */

	inited = true;
}

/* Begin - SRS-PLSQL-SUBPROC */
/*
 * unregister internel func
 */
void
_PG_fini(void)
{
	plisql_unregister_internal_func();

	/* ReqID:SRS-PLSQL-AUTONOMOUSTRANSACTION */
	RegisterAutonxIsoVars_PlisqlHook = prev_RegisterAutonxIsoVars_PlisqlHook;
}
/* End - SRS-PLSQL-SUBPROC */


/* ----------
 * plisql_call_handler
 *
 * The PostgreSQL function manager and trigger manager
 * call this function for execution of PL/iSQL procedures.
 * ----------
 *
 * ReqID:SRS-PLSQL-AUTONOMOUSTRANSACTION
 *   Support autonomous transaction.
 */
PG_FUNCTION_INFO_V1(plisql_call_handler);

Datum
plisql_call_handler(PG_FUNCTION_ARGS)
{
	bool		nonatomic;
	PLiSQL_function *func;
	PLiSQL_execstate *save_cur_estate;
	ResourceOwner procedure_resowner;
	volatile Datum retval = (Datum) 0;
	int			rc;
	/* Begin - SRS-PLSQL-SUBPROC */
	char	function_from = plisql_function_from(fcinfo);
	/* End - SRS-PLSQL-SUBPROC */
	MemoryContext	ax_oldcontext = NULL;
	ResourceOwner	ax_oldresowner= NULL;
	bool			autonomous = false;

	nonatomic = fcinfo->context &&
		IsA(fcinfo->context, CallContext) &&
		!castNode(CallContext, fcinfo->context)->atomic;

	if (!nonatomic && IsTopAutonTransaction())
		nonatomic = true;

	/*
	 * Connect to SPI manager
	 */
	if ((rc = SPI_connect_ext(nonatomic ? SPI_OPT_NONATOMIC : 0)) != SPI_OK_CONNECT)
		elog(ERROR, "SPI_connect failed: %s", SPI_result_code_string(rc));

	/* Find or compile the function */
	/* Begin - SRS-PLSQL-SUBPROC */
	if (function_from == FUNC_FROM_SUBPROCFUNC)
		func = plisql_get_subproc_func(fcinfo, false);
	/* Begin - ReqID:SRS-SQL-PACKAGE */
	else if (function_from == FUNC_FROM_PACKAGE)
	{
		func = plisql_get_package_func(fcinfo, false);
	}
	/* End - ReqID:SRS-SQL-PACKAGE */
	else
		func = plisql_compile(fcinfo, false);
	/* End - nSRS-PLSQL-SUBPROC */

	/* Begin - SRS-PLSQL-SUBPROC */
	/* rember current func in SPI */
	SPI_remember_func(func);
	/* End - SRS-PLSQL-SUBPROC */

	/* Begin - ReqID:SRS-PLSQL-CURSOR */
	if (SPI_is_first_plisql())
	{
		plisql_init_sql_cursor();
		/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
		init_plisql_error_stack();
		/* End - ReqID:SRS-PLSQL-EXCEPTION */
	}
	/* End - ReqID:SRS-PLSQL-CURSOR */

	/* Must save and restore prior value of cur_estate */
	save_cur_estate = func->cur_estate;

	/* Mark the function as busy, so it can't be deleted from under us */
	func->use_count++;

	/*
	 * Startup autonomous transaction if necessary.
	 */
	if (func->autonomous && AutonxEnabled && !RecoveryInProgress())
	{
		ax_oldcontext = CurrentMemoryContext;
		ax_oldresowner = CurrentResourceOwner;

		if (!nonatomic)
		{
			SPI_switch_atomic(true);
			nonatomic = true;
		}

		PG_TRY();
		{
			AutonPrestartPortals();
			ReserveAutoTransactionProc();
			BeginInternalAutonTransaction(func->fn_signature);
		}
		PG_CATCH();
		{
			ReleaseAutoTransactionProc();
			PG_RE_THROW();
		}
		PG_END_TRY();

		MemoryContextSwitchTo(ax_oldcontext);
		//CurrentResourceOwner = ax_oldresowner;
		autonomous = true;
	}

	/*
	 * If we'll need a procedure-lifespan resowner to execute any CALL or DO
	 * statements, create it now.  Since this resowner is not tied to any
	 * parent, failing to free it would result in process-lifespan leaks.
	 * Therefore, be very wary of adding any code between here and the PG_TRY
	 * block.
	 */
	procedure_resowner =
		(nonatomic && func->requires_procedure_resowner) ?
		ResourceOwnerCreate(NULL, "PL/iSQL procedure resources") : NULL;

	PG_TRY();
	{
		/*
		 * Determine if called as function or trigger and call appropriate
		 * subhandler
		 */
		if (CALLED_AS_TRIGGER(fcinfo))
			retval = PointerGetDatum(plisql_exec_trigger(func,
														 (TriggerData *)fcinfo->context,
														!nonatomic));
		else if (CALLED_AS_EVENT_TRIGGER(fcinfo))
		{
			plisql_exec_event_trigger(func,
									  (EventTriggerData *) fcinfo->context,
									  !nonatomic);
			/* there's no return value in this case */
		}
		else
			retval = plisql_exec_function(func, fcinfo,
										   NULL, NULL,
										   procedure_resowner,
										   !nonatomic);
	}
	PG_FINALLY();
	{
		/*
		 * Finish autonomous tansaction if necessary.
		 */
		if (autonomous)
		{
			Assert(IsAutonTransaction() && ax_oldcontext != NULL && ax_oldresowner != NULL);

			if (_do_rethrow)
				RollbackAndReleaseCurrentAutonTransaction();
			else
				ReleaseCurrentAutonTransaction();

			MemoryContextSwitchTo(ax_oldcontext);
			CurrentResourceOwner = ax_oldresowner;
		}

		/* Decrement use-count, restore cur_estate */
		func->use_count--;
		func->cur_estate = save_cur_estate;

		/* Begin - ReqID:SRS-SQL-PACKAGE */
		if (function_from == FUNC_FROM_PACKAGE)
		{
			/* Begin - BUG#M000088 */
			release_package_func_usecount(fcinfo);
			/* End - BUG#M000088 */
		}
		/* End - ReqID:SRS-SQL-PACKAGE */

		/* Be sure to release the procedure resowner if any */
		if (procedure_resowner)
		{
			ResourceOwnerReleaseAllPlanCacheRefs(procedure_resowner);
			ResourceOwnerDelete(procedure_resowner);
		}
	}
	PG_END_TRY();

	/*
	 * Disconnect from SPI manager
	 */
	if ((rc = SPI_finish()) != SPI_OK_FINISH)
		elog(ERROR, "SPI_finish failed: %s", SPI_result_code_string(rc));

	/* Begin - ReqID:SRS-PLSQL-DBMS_OUTPUT */
	if(enable_serveroutput && dbmsoutput_sendbuffer_hook && SPI_get_connected() < 0)
	{
		(*dbmsoutput_sendbuffer_hook)();
	}
	/* End - ReqID:SRS-PLSQL-DBMS_OUTPUT */

	return retval;
}

/* ----------
 * plisql_inline_handler
 *
 * Called by PostgreSQL to execute an anonymous code block
 * ----------
 *
 * ReqID:SRS-PLSQL-AUTONOMOUSTRANSACTION
 *   Support autonomous transaction.
 */
PG_FUNCTION_INFO_V1(plisql_inline_handler);

Datum
plisql_inline_handler(PG_FUNCTION_ARGS)
{
	LOCAL_FCINFO(fake_fcinfo, FUNC_MAX_ARGS);
	InlineCodeBlock *codeblock = castNode(InlineCodeBlock, DatumGetPointer(PG_GETARG_DATUM(0)));
	PLiSQL_function *func;
	FmgrInfo	flinfo;
	EState	   *simple_eval_estate;
	ResourceOwner simple_eval_resowner;
	Datum		retval;
	int			rc;
	MemoryContext oldcontext = CurrentMemoryContext; /* ReqID:SRS-SQL-CALL */
	MemoryContext	ax_oldcontext = NULL;
	ResourceOwner	ax_oldresowner = NULL;
	bool			autonomous = false;

	/* Set codeblock atomic which was in autonomous transaction. */
	if (codeblock->atomic && IsTopAutonTransaction())
		codeblock->atomic = false;

	/*
	 * Connect to SPI manager
	 */
	if ((rc = SPI_connect_ext(codeblock->atomic ? 0 : SPI_OPT_NONATOMIC)) != SPI_OK_CONNECT)
		elog(ERROR, "SPI_connect failed: %s", SPI_result_code_string(rc));

	/* Begin - ReqID:SRS-SQL-ANONYOUT */
	if (codeblock->params != NULL)
	{
		if (codeblock->params->paramnames != NULL)
			set_bindbyname(true);
	}

	/* Compile the anonymous code block */
	func = plisql_compile_inline(codeblock->source_text, codeblock->params/* ReqID:SRS-SQL-ANONYOUT */, codeblock->do_from_call /* ReqID:SRS-SQL-CALL */);

	/* Begin - SRS-PLSQL-SUBPROC */
	/* rember current func in SPI */
	SPI_remember_func(func);
	/* End - SRS-PLSQL-SUBPROC */

	/* Begin - ReqID:SRS-PLSQL-CURSOR */
	if (SPI_is_first_plisql())
	{
		plisql_init_sql_cursor();
		/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
		init_plisql_error_stack();
		/* End - ReqID:SRS-PLSQL-EXCEPTION */
	}
	/* End - ReqID:SRS-PLSQL-CURSOR */

	/* Begin - ReqID:SRS-SQL-ANONYOUT */
	/* check bound variables */
	if (getdynmaic_docheckvar())
	{
		PG_TRY();
		{
			check_variables_does_match(codeblock->params == NULL ? 0 : codeblock->params->numParams);
		}
		PG_CATCH();
		{
			plisql_subxact_cb(SUBXACT_EVENT_ABORT_SUB,
							GetCurrentSubTransactionId(),
							0, NULL);
			/* ... so we can free subsidiary storage */
			plisql_free_function_memory(func, 0, 0);

			/* And propagate the error */
			PG_RE_THROW();
		}
		PG_END_TRY();
	}
	/* End - ReqID:SRS-SQL-ANONYOUT */

	/* Mark the function as busy, just pro forma */
	func->use_count++;

	/* Begin - BUG#Z0001268 */
	/* numParams may be exceeds FUNC_MAX_ARGS */
	if (codeblock->params != NULL &&
		codeblock->params->numParams > FUNC_MAX_ARGS)
	{
		fake_fcinfo = (FunctionCallInfo) palloc(SizeForFunctionCallInfo(codeblock->params->numParams));
	}
	/* End - BUG#Z00001268 */

	/*
	 * Set up a fake fcinfo with just enough info to satisfy
	 * plisql_exec_function().  In particular note that this sets things up
	 * with no arguments passed.
	 */
	/* Begin - ReqID:SRS-SQL-ANONYOUT */
	if (codeblock->params)
		MemSet(fake_fcinfo, 0, SizeForFunctionCallInfo(codeblock->params->numParams));
	else
		MemSet(fake_fcinfo, 0, SizeForFunctionCallInfo(0));
	/* End - ReqID:SRS-SQL-ANONYOUT */

	MemSet(&flinfo, 0, sizeof(flinfo));
	fake_fcinfo->flinfo = &flinfo;
	flinfo.fn_oid = InvalidOid;
	flinfo.fn_mcxt = CurrentMemoryContext;

	/*
	 * Create a private EState and resowner for simple-expression execution.
	 * Notice that these are NOT tied to transaction-level resources; they
	 * must survive any COMMIT/ROLLBACK the DO block executes, since we will
	 * unconditionally try to clean them up below.  (Hence, be wary of adding
	 * anything that could fail between here and the PG_TRY block.)  See the
	 * comments for shared_simple_eval_estate.
	 *
	 * Because this resowner isn't tied to the calling transaction, we can
	 * also use it as the "procedure" resowner for any CALL statements.  That
	 * helps reduce the opportunities for failure here.
	 */
	simple_eval_estate = CreateExecutorState();
	simple_eval_resowner =
		ResourceOwnerCreate(NULL, "PL/iSQL DO block simple expressions");

	/* Begin - ReqID:SRS-SQL-ANONYOUT */
	if (codeblock->params)
	{
		int		i;

		fake_fcinfo->nargs = codeblock->params->numParams;
		for (i = 0; i < codeblock->params->numParams; ++i)
		{
			fake_fcinfo->args[i].value = codeblock->params->params[i].value;
			fake_fcinfo->args[i].isnull = codeblock->params->params[i].isnull;
		}
	}
	/* End - ReqID:SRS-SQL-ANONYOUT */

	/* Begin - BUG#M0000316 */
	reset_oraparam_curpos();
	/* End - BUG#M0000316 */

	/*
	 * Startup autonomous transaction if necessary.
	 */
	if (func->autonomous && AutonxEnabled && !RecoveryInProgress())
	{
		ax_oldcontext = CurrentMemoryContext;
		ax_oldresowner = CurrentResourceOwner;

		if (codeblock->atomic)
		{
			SPI_switch_atomic(true);
			codeblock->atomic = false;
		}

		PG_TRY();
		{
			AutonPrestartPortals();
			ReserveAutoTransactionProc();
			BeginInternalAutonTransaction(func->fn_signature);
		}
		PG_CATCH();
		{
			ReleaseAutoTransactionProc();
			PG_RE_THROW();
		}
		PG_END_TRY();

		MemoryContextSwitchTo(ax_oldcontext);
		//CurrentResourceOwner = ax_oldresowner;
		autonomous = true;
	}

	/* And run the function */
	PG_TRY();
	{
		retval = plisql_exec_function(func, fake_fcinfo,
									   simple_eval_estate,
									   simple_eval_resowner,
									   simple_eval_resowner,	/* see above */
									   codeblock->atomic);

		if (autonomous)
		{
			Assert(IsAutonTransaction() && ax_oldcontext != NULL && ax_oldresowner != NULL);

			ReleaseCurrentAutonTransaction();
			MemoryContextSwitchTo(ax_oldcontext);
			CurrentResourceOwner = ax_oldresowner;
		}
	}
	PG_CATCH();
	{
		/*
		 * Finish autonomous tansaction if necessary.
		 */
		if (autonomous)
		{
			Assert(IsAutonTransaction() && ax_oldcontext != NULL && ax_oldresowner != NULL);

			RollbackAndReleaseCurrentAutonTransaction();
			MemoryContextSwitchTo(ax_oldcontext);
			CurrentResourceOwner = ax_oldresowner;
		}

		/* Begin - ReqID:SRS-SQL-CALL */
		if (codeblock->do_from_call && oldcontext != ErrorContext)
		{
			ErrorData  *edata;

			/* Save error info */
			MemoryContextSwitchTo(oldcontext);
			edata = CopyErrorData();
			FlushErrorState();
			
			/* For CALL statements expect only primary error message */
			if (edata->message)
				elog(ERROR, "%s", edata->message);
			else
				ThrowErrorData(edata);
			FreeErrorData(edata);
		}
		/* End - ReqID:SRS-SQL-CALL */

		/* Begin - BUG#Z0001268 */
		if (codeblock->params != NULL &&
			codeblock->params->numParams > FUNC_MAX_ARGS)
			pfree(fake_fcinfo);
		/* End - BUG#Z00001268 */

		/* Begin - ReqID:SRS-SQL-ANONYOUT */
		set_bindbyname(false);
		/* End - ReqID:SRS-SQL-ANONYOUT */

		/*
		 * We need to clean up what would otherwise be long-lived resources
		 * accumulated by the failed DO block, principally cached plans for
		 * statements (which can be flushed by plisql_free_function_memory),
		 * execution trees for simple expressions, which are in the private
		 * EState, and cached-plan refcounts held by the private resowner.
		 *
		 * Before releasing the private EState, we must clean up any
		 * simple_econtext_stack entries pointing into it, which we can do by
		 * invoking the subxact callback.  (It will be called again later if
		 * some outer control level does a subtransaction abort, but no harm
		 * is done.)  We cheat a bit knowing that plisql_subxact_cb does not
		 * pay attention to its parentSubid argument.
		 */
		plisql_subxact_cb(SUBXACT_EVENT_ABORT_SUB,
						   GetCurrentSubTransactionId(),
						   0, NULL);

		/* Clean up the private EState and resowner */
		FreeExecutorState(simple_eval_estate);
		ResourceOwnerReleaseAllPlanCacheRefs(simple_eval_resowner);
		ResourceOwnerDelete(simple_eval_resowner);

		/* Function should now have no remaining use-counts ... */
		func->use_count--;
		Assert(func->use_count == 0);

		/* ... so we can free subsidiary storage */
		plisql_free_function_memory(func, 0, 0); /* SRS-PLSQL-SUBPROC */

		/* And propagate the error */
		PG_RE_THROW();
	}
	PG_END_TRY();
	set_bindbyname(false);

	if (codeblock->params != NULL && codeblock->params->haveout)
	{
		int16		typLen;
		bool		typByVal;
		MemoryContext old;
		int			i;

		/* use datumCopy to passes variables,first swith to save memory context */
		old = MemoryContextSwitchTo(Ora_spi_saved_memorycontext());
		for (i = 0; i < fake_fcinfo->nargs; ++i)
		{
			if (!fake_fcinfo->args[i].isnull)
			{
				get_typlenbyval(codeblock->params->params[i].ptype, &typLen, &typByVal);
				/* second, use datumCopy because fack_fcinfo memory will be freed by FreeExecutorState() */
				codeblock->params->params[i].value = datumCopy(fake_fcinfo->args[i].value, typByVal, typLen);
				codeblock->params->params[i].isnull = fake_fcinfo->args[i].isnull;
			}
			else
			{
				codeblock->params->params[i].value = (Datum) 0;
				codeblock->params->params[i].isnull = fake_fcinfo->args[i].isnull;
			}
		}
		/* return to old memory context */
		MemoryContextSwitchTo(old);
	}
	/* End - ReqID:SRS-SQL-ANONYOUT */

	/* Begin - BUG#Z0001268 */
	if (codeblock->params != NULL &&
		codeblock->params->numParams > FUNC_MAX_ARGS)
		pfree(fake_fcinfo);
	/* End - BUG#Z00001268 */

	/* Clean up the private EState and resowner */
	FreeExecutorState(simple_eval_estate);
	ResourceOwnerReleaseAllPlanCacheRefs(simple_eval_resowner);
	ResourceOwnerDelete(simple_eval_resowner);

	/* Function should now have no remaining use-counts ... */
	func->use_count--;
	Assert(func->use_count == 0);

	/* ... so we can free subsidiary storage */
	plisql_free_function_memory(func, 0, 0); /* SRS-PLSQL-SUBPROC */

	/*
	 * Disconnect from SPI manager
	 */
	if ((rc = SPI_finish()) != SPI_OK_FINISH)
		elog(ERROR, "SPI_finish failed: %s", SPI_result_code_string(rc));

	/* Begin - ReqID:SRS-PLSQL-DBMS_OUTPUT */
	if(enable_serveroutput && dbmsoutput_sendbuffer_hook && SPI_get_connected() < 0)
	{
		(*dbmsoutput_sendbuffer_hook)();
	}
	/* End - ReqID:SRS-PLSQL-DBMS_OUTPUT */

	return retval;
}

/* ----------
 * plisql_validator
 *
 * This function attempts to validate a PL/iSQL function at
 * CREATE FUNCTION time.
 * ----------
 */
PG_FUNCTION_INFO_V1(plisql_validator);

Datum
plisql_validator(PG_FUNCTION_ARGS)
{
	Oid			funcoid = PG_GETARG_OID(0);
	HeapTuple	tuple;
	Form_pg_proc proc;
	char		functyptype;
	int			numargs;
	Oid		   *argtypes;
	char	  **argnames;
	char	   *argmodes;
	bool		is_dml_trigger = false;
	bool		is_event_trigger = false;
	int			i;

	if (!CheckFunctionValidatorAccess(fcinfo->flinfo->fn_oid, funcoid))
		PG_RETURN_VOID();

	/* Get the new function's pg_proc entry */
	tuple = SearchSysCache1(PROCOID, ObjectIdGetDatum(funcoid));
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "cache lookup failed for function %u", funcoid);
	proc = (Form_pg_proc) GETSTRUCT(tuple);

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
				 proc->prorettype !=ORARECORDOID &&
				 /* End - ReqID:SRS-PLSQL-RECORD */
				 proc->prorettype != VOIDOID &&
				 !IsPolymorphicType(proc->prorettype))
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("PL/iSQL functions cannot return type %s",
							format_type_be(proc->prorettype))));
	}

	/* Disallow pseudotypes in arguments (either IN or OUT) */
	/* except for RECORD and polymorphic */
	numargs = get_func_arg_info(tuple,
								&argtypes, &argnames, &argmodes);
	for (i = 0; i < numargs; i++)
	{
		if (get_typtype(argtypes[i]) == TYPTYPE_PSEUDO)
		{
			if (argtypes[i] != RECORDOID &&
				/* Begin - ReqID:SRS-PLSQL-RECORD */
				argtypes[i] != ORARECORDOID &&
				/* End - ReqID:SRS-PLSQL-RECORD */
				!IsPolymorphicType(argtypes[i]))
				ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("PL/iSQL functions cannot accept type %s",
								format_type_be(argtypes[i]))));
		}
	}

	/* Postpone body checks if !check_function_bodies */
	if (check_function_bodies)
	{
		LOCAL_FCINFO(fake_fcinfo, 0);
		FmgrInfo	flinfo;
		int			rc;
		TriggerData trigdata;
		EventTriggerData etrigdata;

		/*
		 * Connect to SPI manager (is this needed for compilation?)
		 */
		if ((rc = SPI_connect()) != SPI_OK_CONNECT)
			elog(ERROR, "SPI_connect failed: %s", SPI_result_code_string(rc));

		/*
		 * Set up a fake fcinfo with just enough info to satisfy
		 * plisql_compile().
		 */
		MemSet(fake_fcinfo, 0, SizeForFunctionCallInfo(0));
		MemSet(&flinfo, 0, sizeof(flinfo));
		fake_fcinfo->flinfo = &flinfo;
		flinfo.fn_oid = funcoid;
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

		PG_TRY();
		{
			/* Test-compile the function */
			plisql_compile(fake_fcinfo, true);
		}
		PG_CATCH();
		{
			/*
			 * Disconnect from SPI manager
			 */
			if ((rc = SPI_finish()) != SPI_OK_FINISH)
				elog(ERROR, "SPI_finish failed: %s", SPI_result_code_string(rc));
			ReleaseSysCache(tuple);

			PG_RE_THROW();
		}
		PG_END_TRY();
		
		/*
		 * Disconnect from SPI manager
		 */
		if ((rc = SPI_finish()) != SPI_OK_FINISH)
			elog(ERROR, "SPI_finish failed: %s", SPI_result_code_string(rc));
	}

	ReleaseSysCache(tuple);

	PG_RETURN_VOID();
}


/* Begin - BUG#M0000376 */
/* ----------
 * plisql_validator_inline
 *
 * This function attempts to compile a PL/iSQL anonymous block
 * ----------
 */
PG_FUNCTION_INFO_V1(plisql_validator_inline);

Datum
plisql_validator_inline(PG_FUNCTION_ARGS)
{
	InlineCodeBlock *codeblock = castNode(InlineCodeBlock, DatumGetPointer(PG_GETARG_DATUM(0)));
	int 			rc;

	if ((rc = SPI_connect()) != SPI_OK_CONNECT)
		elog(ERROR, "SPI_connect failed: %s", SPI_result_code_string(rc));

	plisql_compile_inline(codeblock->source_text,
						codeblock->params,
						codeblock->do_from_call);

	/*
	 * Disconnect from SPI manager
	 */
	if ((rc = SPI_finish()) != SPI_OK_FINISH)
		elog(ERROR, "SPI_finish failed: %s", SPI_result_code_string(rc));

	PG_RETURN_VOID();
}
/* End - BUG#M0000346 */


/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
/*
 * support sqlcode and sqlerrm are used out of exception block
 */
PG_FUNCTION_INFO_V1(plisql_sqlcode);

Datum
plisql_sqlcode(PG_FUNCTION_ARGS)
{
	int 	sqlcode;

	sqlcode = get_current_sqlcode_sqlerrm(NULL);

	PG_RETURN_INT32(sqlcode);

}


PG_FUNCTION_INFO_V1(plisql_sqlerrm);

Datum
plisql_sqlerrm(PG_FUNCTION_ARGS)
{
	int 	sqlcode;
	char 	*sqlerrm = NULL;

	sqlcode = get_current_sqlcode_sqlerrm(&sqlerrm);

	if (sqlcode == 0)
		PG_RETURN_TEXT_P(cstring_to_text("IVY-0000: normal, successful completion"));
	else if (sqlerrm != NULL)
		PG_RETURN_TEXT_P(cstring_to_text(sqlerrm));
	else
	{
		PG_RETURN_TEXT_P(cstring_to_text(plisql_recognize_err_message_int(sqlcode)));
	}
}

PG_FUNCTION_INFO_V1(plisql_sqlerrm_int4);

Datum
plisql_sqlerrm_int4(PG_FUNCTION_ARGS)
{
	int		sqlcode;

	sqlcode = (int)PG_GETARG_INT32(0);

	PG_RETURN_TEXT_P(cstring_to_text(plisql_recognize_err_message_int(sqlcode)));
}

PG_FUNCTION_INFO_V1(plisql_raise_application_error);


PG_FUNCTION_INFO_V1(plisql_error_stack);

Datum
plisql_error_stack(PG_FUNCTION_ARGS)
{
	char *errmsg;

	errmsg = format_error_stack_msg();

	if (errmsg == NULL)
		PG_RETURN_NULL();
	else
		PG_RETURN_TEXT_P(cstring_to_text(errmsg));
}


Datum
plisql_raise_application_error(PG_FUNCTION_ARGS)
{
	int	sqlcode;
	text	*msg;
	StringInfoData data;
	int	pgsqlcode;
	char *errmsg;
	bool keeperrorstack;

	sqlcode = (int)PG_GETARG_INT32(0);
	msg = PG_GETARG_TEXT_P(1);
	keeperrorstack = PG_GETARG_BOOL(2);
	errmsg = TextDatumGetCString(msg);

	pgsqlcode = plisql_get_exception_pgerrcode(CurrentMemoryContext,
									sqlcode,
									NULL);
	initStringInfo(&data);
	appendStringInfo(&data, "IVY%d: %s", sqlcode, errmsg);
	pfree(errmsg);

	/* raise error */
	ereport(ERROR,
			(errcode(pgsqlcode),
			 errmsg_internal("%s", data.data),
			 err_generic_nokeeperrorstack(keeperrorstack)));
	return (Datum) 0;
}
/* End - ReqID:SRS-PLSQL-EXCEPTION */
