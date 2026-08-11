/*-------------------------------------------------------------------------
 *
 * parse_param.c
 *	  handle parameters in parser
 *
 * This code covers two cases that are used within the core backend:
 *		* a fixed list of parameters with known types
 *		* an expandable list of parameters whose types can optionally
 *		  be determined from context
 * In both cases, only explicit $n references (ParamRef nodes) are supported.
 *
 * Note that other approaches to parameters are possible using the parser
 * hooks defined in ParseState.
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/parser/parse_param.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include <limits.h>

#include "catalog/pg_type.h"
#include "nodes/nodeFuncs.h"
#include "parser/parse_param.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
/* Begin - ReqID:SRS-SQL-FUNCOUT */
#include "catalog/pg_proc.h"
#include "miscadmin.h"
#include "utils/ora_compatible.h"
#include "utils/guc.h"
/* End - ReqID:SRS-SQL-FUNCOUT */
#include "executor/spi.h" /* OA#2707 */

typedef struct FixedParamState
{
	Oid		   *paramTypes;		/* array of parameter type OIDs */
	int			numParams;		/* number of array entries */
} FixedParamState;

/*
 * In the varparams case, the caller-supplied OID array (if any) can be
 * re-palloc'd larger at need.  A zero array entry means that parameter number
 * hasn't been seen, while UNKNOWNOID means the parameter has been used but
 * its type is not yet known.
 */
typedef struct VarParamState
{
	Oid		  **paramTypes;		/* array of parameter type OIDs */
	int		   *numParams;		/* number of array entries */
	/* Begin - ReqID:SRS-SQL-FUNCOUT */
	char 	   **paramModes;	/* array of parameter mode: PROARGMODE_IN,
								 * PROARGMODE_OUT, PROARGMODE_INOUT, PROARGMODE_RESULT.
								 * note that when it's PROARGMODE_RESULT,
								 * only used to represent the funtion result by jdbc.*/
	/* End - ReqID:SRS-SQL-FUNCOUT */
} VarParamState;

/* Begin - ReqID:SRS-SQL-ANONYOUT */
typedef struct OraParamNumber
{
	int 	number;
	char*	name;
	/* Begin - BUG#M0000376 */
	char	mode;
	/* End - BUG#M0000376 */
}OraParamNumber;

typedef struct OraParamNumbers
{
	int  			param_max;
	int   			param_count;
	int             level;
	OraParamNumber* params;
	MemoryContext   mcontext;
	/* Begin - BUG#M0000264 */
	/*
	 * saved previous global variable
	 */
	bool			IsDynParser;
	bool			IsDynDoStmt;
	bool			HasPgParam;
	bool			DoStmtCheckVar;
	bool 			IsDynCallStmt;
	bool			IsBindByName;
	bool			IsInitDyn;
	/* Begin - BUG#M0000316 */
	bool			HasQuestionMark;
	int  			questionmark_max;
	int   			questionmark_count;
	int				*QuestionMarkNumbers;
	int				curpos;
	/* End - BUG#M0000316 */
	/* End - BUG#M0000264 */
	struct OraParamNumbers* prev;
}OraParamNumbers;


/* Begin - BUG#M0000264 */
/*
 * the number of OraParamRef count
 */
List *ora_parameter_list = NIL;
List *ora_parameter_name_list = NIL;
/* End - BUG#M0000264 */


OraParamNumbers *CurrentOraParamNode = NULL;

/*
 * dynamic sql stmt parser status
 * IsDynParser : indicates grammer is parsing dynamic sql.
 * IsDynDoStmt: indicates grammer is parsing dynamic sql which is DoStmt.
 * HasPgParam: indicates grammer found pg model variables like $1...$n.
 * DoStmtCheckVar: indicates we should or not check bound var matching
 * IsBindByName: bind the paramter by name
 * in compile DoStmt, because DoStmt always compile after others stmt.
 * we use DoStmtCheckVar, because we support grammer like this execute 'select xxxx;
 * begin xxx end;begin xxx end;' this grammer we must not check var bound match.
 */
static bool			IsDynParser = false;
static bool			IsDynDoStmt = false; /* BUG#M0000264 */
static bool			HasPgParam = false;
static bool			DoStmtCheckVar = false;
static bool			IsBindByName = false;
/* Begin - ReqID:SRS-SQL-CALL */
static bool			IsDynCallStmt = false; /* ORA-06578: output parameter cannot be a duplicate bind */
/* End - ReqID:SRS-SQL-CALL */
/* Begin - BUG#M0000316 */
static bool			HasQuestionMark = false;
/* End - BUG#M0000316 */
/* End - ReqID:SRS-SQL-ANONYOUT */

/* Begin - BUG#M0000264 */
/*
 * is begin execute immediate ?
 */
static bool			IsInitDyn = false;
/* End - BUG#M0000264 */



static Node *fixed_paramref_hook(ParseState *pstate, ParamRef *pref);
static Node *variable_paramref_hook(ParseState *pstate, ParamRef *pref);
static Node *variable_coerce_param_hook(ParseState *pstate, Param *param,
										Oid targetTypeId, int32 targetTypeMod,
										int location);
static bool check_parameter_resolution_walker(Node *node, ParseState *pstate);
static bool query_contains_extern_params_walker(Node *node, void *context);

/*
 * Set up to process a query containing references to fixed parameters.
 */
void
parse_fixed_parameters(ParseState *pstate,
					   Oid *paramTypes, int numParams)
{
	FixedParamState *parstate = palloc(sizeof(FixedParamState));

	parstate->paramTypes = paramTypes;
	parstate->numParams = numParams;
	pstate->p_ref_hook_state = (void *) parstate;
	pstate->p_paramref_hook = fixed_paramref_hook;
	/* no need to use p_coerce_param_hook */
}

/*
 * Set up to process a query containing references to variable parameters.
 */
void
parse_variable_parameters(ParseState *pstate,
						  Oid **paramTypes, int *numParams)
{
	parse_variable_parameters_ext(pstate, paramTypes, numParams, NULL);
}

void
parse_variable_parameters_ext(ParseState *pstate,
						  Oid **paramTypes, int *numParams,
						  /* Begin - ReqID:SRS-SQL-FUNCOUT */
						  char **paramModes)
						  /* End - ReqID:SRS-SQL-FUNCOUT */
{
	VarParamState *parstate = palloc(sizeof(VarParamState));

	/* Begin - ReqID:SRS-SQL-FUNCOUT */
	/*
	 * get the real paramTypes(reset 32nd-30nd bit of 32 bit Oid)
	 */
	if (ORA_MODE == compatible_mode)
	{
		int 		   i;

		for (i = 0; i < *numParams; i++)
		{
			UnSetModeOut((*paramTypes)[i]);
		}
		pstate->p_isVarParamState = true;
		parstate->paramModes = paramModes;
	}
	else
	{
		pstate->p_isVarParamState = false;
		parstate->paramModes = NULL;
	}
	/* End - ReqID:SRS-SQL-FUNCOUT */

	parstate->paramTypes = paramTypes;
	parstate->numParams = numParams;
	pstate->p_ref_hook_state = (void *) parstate;
	pstate->p_paramref_hook = variable_paramref_hook;
	pstate->p_coerce_param_hook = variable_coerce_param_hook;
}

/*
 * Transform a ParamRef using fixed parameter types.
 */
static Node *
fixed_paramref_hook(ParseState *pstate, ParamRef *pref)
{
	FixedParamState *parstate = (FixedParamState *) pstate->p_ref_hook_state;
	int			paramno = pref->number;
	Param	   *param;

	/* Check parameter number is valid */
	if (paramno <= 0 || paramno > parstate->numParams ||
		!OidIsValid(parstate->paramTypes[paramno - 1]))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_PARAMETER),
				 /* Begin - ReqID:SRS-SQL-ANONYOUT */
				 IsA(pref, OraParamRef) ? errmsg("there is no parameter %s", ((OraParamRef*)pref)->name) :
				 errmsg("there is no parameter $%d", pref->number),
				 /* End - ReqID:SRS-SQL-ANONYOUT */
				 parser_errposition(pstate, pref->location)));

	param = makeNode(Param);
	param->paramkind = PARAM_EXTERN;
	param->paramid = paramno;
	param->paramtype = parstate->paramTypes[paramno - 1];
	param->paramtypmod = -1;
	param->paramcollid = get_typcollation(param->paramtype);
	param->location = pref->location;
	/* Begin - ReqID:SRS-PLSQL-RECORD */
	param->type = NULL;
	/* End - ReqID:SRS-PLSQL-RECORD */

	return (Node *) param;
}

/*
 * Transform a ParamRef using variable parameter types.
 *
 * The only difference here is we must enlarge the parameter type array
 * as needed.
 */
static Node *
variable_paramref_hook(ParseState *pstate, ParamRef *pref)
{
	VarParamState *parstate = (VarParamState *) pstate->p_ref_hook_state;
	int			paramno = pref->number;
	Oid		   *pptype;
	Param	   *param;

	/* Check parameter number is in range */
	if (paramno <= 0 || paramno > INT_MAX / sizeof(Oid))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_PARAMETER),
				 errmsg("there is no parameter $%d", paramno),
				 parser_errposition(pstate, pref->location)));
	if (paramno > *parstate->numParams)
	{
		/* Need to enlarge param array */
		if (*parstate->paramTypes)
			*parstate->paramTypes = (Oid *) repalloc(*parstate->paramTypes,
													 paramno * sizeof(Oid));
		else
			*parstate->paramTypes = (Oid *) palloc(paramno * sizeof(Oid));
		/* Zero out the previously-unreferenced slots */
		MemSet(*parstate->paramTypes + *parstate->numParams,
			   0,
			   (paramno - *parstate->numParams) * sizeof(Oid));

		/* Begin - ReqID:SRS-SQL-FUNCOUT */
		if (ORA_MODE == compatible_mode && parstate->paramModes != NULL)
		{
			/* Need to enlarge param mode array */
			if (*parstate->paramModes)
				*parstate->paramModes = (char *) repalloc(*parstate->paramModes,
														 paramno * sizeof(char));
			else
				*parstate->paramModes = (char *) palloc(paramno * sizeof(char));
			/* set the previously-unreferenced slots paramModes to INOUT*/
			MemSet(*parstate->paramModes + *parstate->numParams,
				   PROARGMODE_IN,
				   (paramno - *parstate->numParams) * sizeof(char));
		}
		/* End - ReqID:SRS-SQL-FUNCOUT */

		*parstate->numParams = paramno;
	}

	/* Locate param's slot in array */
	pptype = &(*parstate->paramTypes)[paramno - 1];

	/* If not seen before, initialize to UNKNOWN type */
	if (*pptype == InvalidOid)
		*pptype = UNKNOWNOID;

	/*
	 * If the argument is of type void and it's procedure call, interpret it
	 * as unknown.  This allows the JDBC driver to not have to distinguish
	 * function and procedure calls.  See also another component of this hack
	 * in ParseFuncOrColumn().
	 */
	if (*pptype == VOIDOID && pstate->p_expr_kind == EXPR_KIND_CALL_ARGUMENT)
		*pptype = UNKNOWNOID;

	param = makeNode(Param);
	param->paramkind = PARAM_EXTERN;
	param->paramid = paramno;
	param->paramtype = *pptype;
	param->paramtypmod = -1;
	param->paramcollid = get_typcollation(param->paramtype);
	param->location = pref->location;
	/* Begin - ReqID:SRS-PLSQL-RECORD */
	param->type = NULL;
	/* End - ReqID:SRS-PLSQL-RECORD */

	return (Node *) param;
}

/*
 * Coerce a Param to a query-requested datatype, in the varparams case.
 */
static Node *
variable_coerce_param_hook(ParseState *pstate, Param *param,
						   Oid targetTypeId, int32 targetTypeMod,
						   int location)
{
	if (param->paramkind == PARAM_EXTERN && param->paramtype == UNKNOWNOID)
	{
		/*
		 * Input is a Param of previously undetermined type, and we want to
		 * update our knowledge of the Param's type.
		 */
		VarParamState *parstate = (VarParamState *) pstate->p_ref_hook_state;
		Oid		   *paramTypes = *parstate->paramTypes;
		int			paramno = param->paramid;

		if (paramno <= 0 ||		/* shouldn't happen, but... */
			paramno > *parstate->numParams)
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_PARAMETER),
					 errmsg("there is no parameter $%d", paramno),
					 parser_errposition(pstate, param->location)));

		if (paramTypes[paramno - 1] == UNKNOWNOID)
		{
			/* We've successfully resolved the type */
			paramTypes[paramno - 1] = targetTypeId;
		}
		else if (paramTypes[paramno - 1] == targetTypeId)
		{
			/* We previously resolved the type, and it matches */
		}
		else
		{
			/* Oops */
			ereport(ERROR,
					(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
					 errmsg("inconsistent types deduced for parameter $%d",
							paramno),
					 errdetail("%s versus %s",
							   format_type_be(paramTypes[paramno - 1]),
							   format_type_be(targetTypeId)),
					 parser_errposition(pstate, param->location)));
		}

		param->paramtype = targetTypeId;

		/*
		 * Note: it is tempting here to set the Param's paramtypmod to
		 * targetTypeMod, but that is probably unwise because we have no
		 * infrastructure that enforces that the value delivered for a Param
		 * will match any particular typmod.  Leaving it -1 ensures that a
		 * run-time length check/coercion will occur if needed.
		 */
		param->paramtypmod = -1;

		/*
		 * This module always sets a Param's collation to be the default for
		 * its datatype.  If that's not what you want, you should be using the
		 * more general parser substitution hooks.
		 */
		param->paramcollid = get_typcollation(param->paramtype);

		/* Use the leftmost of the param's and coercion's locations */
		if (location >= 0 &&
			(param->location < 0 || location < param->location))
			param->location = location;

		return (Node *) param;
	}

	/* Else signal to proceed with normal coercion */
	return NULL;
}

/*
 * Check for consistent assignment of variable parameters after completion
 * of parsing with parse_variable_parameters.
 *
 * Note: this code intentionally does not check that all parameter positions
 * were used, nor that all got non-UNKNOWN types assigned.  Caller of parser
 * should enforce that if it's important.
 */
void
check_variable_parameters(ParseState *pstate, Query *query)
{
	VarParamState *parstate = (VarParamState *) pstate->p_ref_hook_state;

	/* If numParams is zero then no Params were generated, so no work */
	if (*parstate->numParams > 0)
		(void) query_tree_walker(query,
								 check_parameter_resolution_walker,
								 (void *) pstate, 0);
}

/*
 * Traverse a fully-analyzed tree to verify that parameter symbols
 * match their types.  We need this because some Params might still
 * be UNKNOWN, if there wasn't anything to force their coercion,
 * and yet other instances seen later might have gotten coerced.
 */
static bool
check_parameter_resolution_walker(Node *node, ParseState *pstate)
{
	if (node == NULL)
		return false;
	if (IsA(node, Param))
	{
		Param	   *param = (Param *) node;

		if (param->paramkind == PARAM_EXTERN)
		{
			VarParamState *parstate = (VarParamState *) pstate->p_ref_hook_state;
			int			paramno = param->paramid;

			if (paramno <= 0 || /* shouldn't happen, but... */
				paramno > *parstate->numParams)
				ereport(ERROR,
						(errcode(ERRCODE_UNDEFINED_PARAMETER),
						 errmsg("there is no parameter $%d", paramno),
						 parser_errposition(pstate, param->location)));

			if (param->paramtype != (*parstate->paramTypes)[paramno - 1])
				ereport(ERROR,
						(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
						 errmsg("could not determine data type of parameter $%d",
								paramno),
						 parser_errposition(pstate, param->location)));
		}
		return false;
	}
	if (IsA(node, Query))
	{
		/* Recurse into RTE subquery or not-yet-planned sublink subquery */
		return query_tree_walker((Query *) node,
								 check_parameter_resolution_walker,
								 (void *) pstate, 0);
	}
	return expression_tree_walker(node, check_parameter_resolution_walker,
								  (void *) pstate);
}

/*
 * Check to see if a fully-parsed query tree contains any PARAM_EXTERN Params.
 */
bool
query_contains_extern_params(Query *query)
{
	return query_tree_walker(query,
							 query_contains_extern_params_walker,
							 NULL, 0);
}

static bool
query_contains_extern_params_walker(Node *node, void *context)
{
	if (node == NULL)
		return false;
	if (IsA(node, Param))
	{
		Param	   *param = (Param *) node;

		if (param->paramkind == PARAM_EXTERN)
			return true;
		return false;
	}
	if (IsA(node, Query))
	{
		/* Recurse into RTE subquery or not-yet-planned sublink subquery */
		return query_tree_walker((Query *) node,
								 query_contains_extern_params_walker,
								 context, 0);
	}
	return expression_tree_walker(node, query_contains_extern_params_walker,
								  context);
}

/* Begin - ReqID:SRS-SQL-FUNCOUT */
Node *
ParseParamVariable(Node *arg)
{
	Node *result = arg;
	if (IsA(arg, Param))
		return result;

	if (IsA(arg, RelabelType))
	{
		result = (Node *)((RelabelType *)arg)->arg;
		return ParseParamVariable(result);
	}
	else if (IsA(arg, FuncExpr))
	{
		List *args = ((FuncExpr *)arg)->args;
		if (((FuncExpr*)arg)->funcformat != COERCE_EXPLICIT_CALL)
			return ParseParamVariable((Node *)linitial(args));
	}
	else if (IsA(arg, CoerceViaIO))
	{
		result = (Node *) ((CoerceViaIO *)arg)->arg;
		return ParseParamVariable(result);
	}
	/* Begin - ReqID:SRS-SQL-PACKAGE */
	else if (IsA(arg, ArrayCoerceExpr))
	{
		result = (Node *) ((ArrayCoerceExpr *)arg)->arg;
		return ParseParamVariable(result);
	}
	/* End - ReqID:SRS-SQL-PACKAGE */

	return result;
}

/*
 * GetParamMode
 * get the parameter type mode from the type Oid' 32nd-30nd bit.
 */
char *
GetParamMode(Oid *paramTypes, int numParams)
{
	int		i;
	char	*modes = NULL; /* BUG:M0000083 */

	for (i = 0; i < numParams; i++)
	{
		/* Begin - BUG:M0000083 */
		if ((paramTypes[i] & 0xE0000000) == 0 && modes == NULL)
			continue;

		if (modes == NULL)
		{
			modes = palloc0(sizeof(char) * numParams);

			if (i > 0)
				MemSet(modes, PROARGMODE_IN, i);
		}
		/* End - BUG:M0000083 */

		if (IsModeIn(paramTypes[i]) && IsModeOut(paramTypes[i]))
			modes[i] = PROARGMODE_INOUT;
		else if (IsModeOut(paramTypes[i]))
			modes[i] = PROARGMODE_OUT;
		else
			modes[i] = PROARGMODE_IN;
	}

	return modes;
}

/*
 * ParseVarParamState
 * Parse the params.
 * If it is '{ ? = call func/proc}' form jdbc, set isLeftCall to true
 */
void
ParseVarParamState(void *arg, Node *param,
				bool *firstOutIsReturn, bool proc_call)
{
	int 		   index;
	char 		   mode;
	VarParamState *vps = (VarParamState *) arg;
	char 		  *modes;

	if (vps->paramModes == NULL)
		return;

	modes = *vps->paramModes;
	if (!modes)
		return;

	index = ((Param *) param)->paramid - 1;
	mode = modes[index];

	if (where_from == FROM_JDBC &&
		!proc_call &&
		/* Begin - OA#2707 */
		index == 0 &&
		SPI_get_connected() < 0 &&
		/* End - OA#2707 */
		firstOutIsReturn != NULL &&
		mode == PROARGMODE_OUT)
		*firstOutIsReturn = true;

	/*
	 * OA#2707:
	 * Here, we need to set the OID of the return parameter to
	 * VOIDOID because we cannot distinguish between return parameters and
	 * `out` paramters. in the case of a PBE call, the 'P' phase identifies the first
	 * paramter as a return parameter, but does not set its OID to 'VOIDOI` if the execution
	 * plan becomes invalid during the 'B' or 'E' phase and requires re-parsing, the sysstem will
	 * be unable to distinguish whether the first parameter is a return parameter or an `out`
	 * parameter, leading to  function parsing failure
	 */
	if (*firstOutIsReturn)
		(*vps->paramTypes)[index] = VOIDOID;
}
/* End - ReqID:SRS-SQL-FUNCOUT */

/* Begin - ReqID:SRS-SQL-ANONYOUT */
int
push_oraparam_stack(void)
{
	OraParamNumbers *new;

	new = palloc0(sizeof(OraParamNumbers));
	new->params = palloc0(sizeof(OraParamNumber)* 8);
	new->param_max = 8;
	new->mcontext = CurrentMemoryContext;

	/* Begin - BUG#M0000316 */
	new->QuestionMarkNumbers = palloc0(sizeof(int)* 8);
	new->questionmark_max = 8;
	/* End - BUG#M0000316 */

	/* Begin - BUG#M0000264 */
	if (CurrentOraParamNode == NULL)
	{
		new->level = 1;
	}
	else
	{
		/* Begin - BUG#M0000264 */
		CurrentOraParamNode->IsDynParser = IsDynParser;
		CurrentOraParamNode->IsDynDoStmt = IsDynDoStmt;
		CurrentOraParamNode->HasPgParam = HasPgParam;
		CurrentOraParamNode->DoStmtCheckVar = DoStmtCheckVar;
		CurrentOraParamNode->IsDynCallStmt = IsDynCallStmt;
		CurrentOraParamNode->IsInitDyn = IsInitDyn;
		CurrentOraParamNode->IsBindByName = IsBindByName;
		/* Begin - BUG#M0000316 */
		CurrentOraParamNode->HasQuestionMark = HasQuestionMark;
		/* End - BUG#M0000316 */

		new->level = CurrentOraParamNode->level + 1;
	}
	new->prev = CurrentOraParamNode;
	CurrentOraParamNode = new;

	/*
	 * init global variable value
	 */
	IsDynParser = true;
	IsInitDyn = true;
	IsDynDoStmt = false;
	HasPgParam = false;
	IsDynCallStmt = false;
	DoStmtCheckVar = false;
	IsBindByName = false;
	/* End - BUG#M0000264 */
	/* Begin - BUG#M0000316 */
	HasQuestionMark = false;
	/* End - BUG#M0000316 */

	return new->level - 1;
}


void
pop_oraparam_stack(int cur)
{
	while (CurrentOraParamNode != NULL && cur >= 0 && CurrentOraParamNode->level > cur)
	{
		int i;
		OraParamNumbers *oraparams = CurrentOraParamNode->prev;

		for (i = 0; i < CurrentOraParamNode->param_count; ++i)
		{
			pfree(CurrentOraParamNode->params[i].name);
		}
		
		pfree(CurrentOraParamNode->params);
		/* Begin - BUG#M0000316 */
		pfree(CurrentOraParamNode->QuestionMarkNumbers);
		/* End - BUG#M0000316 */
		pfree(CurrentOraParamNode);

		CurrentOraParamNode = oraparams;
	}
	if (CurrentOraParamNode != NULL)
	{
		IsDynParser = CurrentOraParamNode->IsDynParser;
		IsDynDoStmt = CurrentOraParamNode->IsDynDoStmt;
		HasPgParam = CurrentOraParamNode->HasPgParam;
		DoStmtCheckVar = CurrentOraParamNode->DoStmtCheckVar;
		IsDynCallStmt = CurrentOraParamNode->IsDynCallStmt;
		IsInitDyn = CurrentOraParamNode->IsInitDyn;
		IsBindByName = CurrentOraParamNode->IsBindByName;
		/* Begin - BUG#M0000316 */
		HasQuestionMark = CurrentOraParamNode->HasQuestionMark;
		/* End - BUG#M0000316 */
	}
	else
	{
		/* init value */
		IsDynParser = false;
		IsDynDoStmt = false;
		HasPgParam = false;
		DoStmtCheckVar = false;
		IsDynCallStmt = false;
		IsInitDyn = false;
		IsBindByName= false;
		/* Begin - BUG#M0000316 */
		HasQuestionMark = false;
		/* End - BUG#M0000316 */
	}
}


int
calculate_oraparamnumber(const char* name)
{
	 int i;
	 int number = 0;
	 MemoryContext oldcontext;
	 if (CurrentOraParamNode == NULL)
		 return 0;
	 
	 /* dynamic sql handle the same name bindvar */
	 if (IsDynDoStmt || IsDynCallStmt) /* BUG#M0000264 */
	 {
		 for (i = 0; i < CurrentOraParamNode->param_count; ++i)
		 {
			 if (strcmp(CurrentOraParamNode->params[i].name, name) == 0)
			 {
			 	/* Begin - ReqID:SRS-SQL-CALL */
				/* ORA-06578: output parameter cannot be a duplicate bind */
			 	if (IsDynCallStmt)
					ereport(ERROR,
							(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
							 errmsg("output parameter cannot be a duplicate bind")));
				else
				/* End - ReqID:SRS-SQL-CALL */
					return CurrentOraParamNode->params[i].number;
			 }
		 }
	 }
	 
	 if (CurrentOraParamNode->param_count >= CurrentOraParamNode->param_max)
	 {
		 CurrentOraParamNode->params = (OraParamNumber*) repalloc(CurrentOraParamNode->params, sizeof(OraParamNumber)*CurrentOraParamNode->param_max * 2);
		 CurrentOraParamNode->param_max *= 2;
	 }
	 
	 number = CurrentOraParamNode->param_count + 1;
	 
	 oldcontext = MemoryContextSwitchTo(CurrentOraParamNode->mcontext);
	 CurrentOraParamNode->params[CurrentOraParamNode->param_count].name = pstrdup(name);
	 /* Begin - BUG#M0000376 */
	 CurrentOraParamNode->params[CurrentOraParamNode->param_count].mode = PROARGMODE_NOT_SET;
	 /* End - BUG#M0000376 */
	 MemoryContextSwitchTo(oldcontext);
	 
	 CurrentOraParamNode->params[CurrentOraParamNode->param_count++].number = number;	 
	 return number;  
}

/* Begin - BUG#M0000316 */
int
pl_calculate_questionmarknumber(void)
{
	int number = 0;

	if (CurrentOraParamNode == NULL)
		return 0;

	if (HasPgParam)
		ereport(ERROR,
				(errcode(ERRCODE_SYSTEM_ERROR),
				 errmsg("can't use $n and ? bindvar model at the same time")));

	if (CurrentOraParamNode->params != NULL &&
		CurrentOraParamNode->param_count > 0)
		ereport(ERROR,
				(errcode(ERRCODE_SYSTEM_ERROR),
				 errmsg("can't use ? and :x bindvar model at the same time")));

	if (CurrentOraParamNode->questionmark_count >= CurrentOraParamNode->questionmark_max)
	{
		CurrentOraParamNode->QuestionMarkNumbers = (int *) repalloc(CurrentOraParamNode->QuestionMarkNumbers, sizeof(int)* CurrentOraParamNode->questionmark_max * 2);
		CurrentOraParamNode->questionmark_max *= 2;	/* BUG:Z1661 */
	}

	number = CurrentOraParamNode->questionmark_count + 1;

	CurrentOraParamNode->QuestionMarkNumbers[CurrentOraParamNode->questionmark_count++] = number;

	return number;
}

int
calculate_questionmarknumber(void)
{
	int number = 0;

	if (CurrentOraParamNode == NULL)
		return 0;

	if (HasPgParam)
		ereport(ERROR,
				(errcode(ERRCODE_SYSTEM_ERROR),
				 errmsg("can't use $n and ? bindvar model at the same time")));

	if (CurrentOraParamNode->params != NULL &&
		CurrentOraParamNode->param_count > 0)
		ereport(ERROR,
				(errcode(ERRCODE_SYSTEM_ERROR),
				 errmsg("can't use ? and :x bindvar model at the same time")));

	number = CurrentOraParamNode->QuestionMarkNumbers[CurrentOraParamNode->curpos];
	CurrentOraParamNode->curpos++;

	return number;
}

void
reset_oraparam_curpos(void)
{
	if (CurrentOraParamNode == NULL)
	 return;

	CurrentOraParamNode->curpos = 0;
}

/* End - BUG#M0000316 */

/* Begin - BUG#M0000264 */
/*
 *recalculate oraparamnumber for special stmt
 */
void
recalculate_oraparamnumber(List *parsetree_list)
{
	ListCell *cell_item;
	Node *first_tree = NULL;

	/* paramNode is null */
	if (CurrentOraParamNode == NULL)
		return;

	if (parsetree_list == NIL)
		return;

	first_tree = (Node *) lfirst(list_head(parsetree_list));

	/* handle DoStmt */
	if (IsInitDyn &&
		(list_length(parsetree_list) != 1 ||
		 ((nodeTag(first_tree) == T_RawStmt) &&
		 nodeTag(((RawStmt *) first_tree)->stmt) == T_DoStmt)))
		 setdynamic_doparser(true);

	/*
	 * not When DoStmt for trans
	 * callStmt into DoStmt
	 */
	if (IsInitDyn &&
		!IsDynDoStmt &&
		list_length(parsetree_list) == 1 &&
		nodeTag(first_tree) == T_RawStmt &&
		nodeTag(((RawStmt *) first_tree)->stmt) == T_CallStmt)
		setdynamic_callparser(true);

	/* reset it */
	if (IsInitDyn)
		IsInitDyn = false;

	/* no DoStmt and more stmts */
	if (IsDynDoStmt)
	{
		/*
		 * DoStmt or more stmt should reassign its number
		 */
		foreach (cell_item, ora_parameter_name_list)
		{
			char *name = (char *) lfirst(cell_item);

			calculate_oraparamnumber(name);
		}
	}

	/*
	 * change ora_parameter_list number
	 */
	foreach(cell_item, ora_parameter_list)
	{
		OraParamRef *oraref = (OraParamRef *) lfirst(cell_item);

		oraref->number = calculate_oraparamnumber(oraref->name);
	}
	return;
}

/*
 * get current oraparam level
 */
int
get_current_oraparam_level(void)
{
	if (CurrentOraParamNode == NULL)
		return 0;
	else
		return CurrentOraParamNode->level;
}

/*
 * calculate oraparamname position
 */
int
recalculate_oraparamname_position(char ***paramnames,
							/* Begin - BUG#M0000376 */
							char **parammodes)
							/* End - BUG#M0000376 */
{
	int numparams = 0;
	int i;
	char **oraparamnames;
	/* Begin - BUG#M0000376 */
	char *oraparammodes = NULL;
	/* End - BUG#M0000376 */

	if (CurrentOraParamNode == NULL)
		return 0;

	numparams = CurrentOraParamNode->param_count;

	if (paramnames == NULL)
		return numparams;

	/* oranumber start from 1 */
	oraparamnames = palloc0(sizeof(char *) * (numparams + 1));
	/* Begin - BUG#M0000376 */
	if (enable_get_oraparam_mode)
		oraparammodes = palloc0(sizeof(char) * (numparams + 1));
	/* End - BUG#M0000376 */

	for (i = 0; i < numparams; i++)
	{
		if (oraparamnames[CurrentOraParamNode->params[i].number] == NULL)
		{
			oraparamnames[CurrentOraParamNode->params[i].number] = pstrdup(CurrentOraParamNode->params[i].name);
			/* Begin - BUG#M0000376 */
			if (enable_get_oraparam_mode)
				oraparammodes[CurrentOraParamNode->params[i].number] = CurrentOraParamNode->params[i].mode;
			/* End - BUG#M0000376 */
		}
		else if (strcmp(oraparamnames[CurrentOraParamNode->params[i].number],
				CurrentOraParamNode->params[i].name) != 0)
			elog(ERROR, "the same position %d, but has different name",
					CurrentOraParamNode->params[i].number);
	}

	/*
	 * check parameter rigth
	 * we should make true all the parameter has a name
	 * the var number start from 1
	 */
	for (i = 1; i <= numparams; i++)
		if (oraparamnames[i] == NULL)
			elog(ERROR, "the position of %d placeholdvar has no name", i);
	*paramnames = oraparamnames;
	/* Begin - BUG#M0000376 */
	if (enable_get_oraparam_mode)
		*parammodes = oraparammodes;
	/* End - BUG#M0000376 */

	return numparams;
}

/* End - BUG#M0000264 */


/*
 * set dynamic parser status
 */
void
setdynamic_parser(bool value)
{
	IsDynParser = value;
}

/*
 * get dynamic parser status
 */
bool
getdynamic_parser(void)
{
	return IsDynParser;
}

/*
 * dynamic sql stmt,check bind variables match
 */
void
check_variables_does_match(int variables)
{
	if (CurrentOraParamNode != NULL &&
		CurrentOraParamNode->param_count > 0 &&
		HasPgParam)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				errmsg("can't use $n and :x bindvar model at the same time")));

	if (!HasPgParam
		/* Begin - BUG#M0000316 */
		&& !HasQuestionMark)
		/* End - BUG#M0000316 */
	{
		if (CurrentOraParamNode == NULL)
		{
			if (variables > 0)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						errmsg("bind variable does not exist")));
		}
		else if (CurrentOraParamNode->param_count > variables)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					errmsg("not all variables bound")));
		else if (CurrentOraParamNode->param_count < variables)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					errmsg("bind variable does not exist")));
	}
	/* Begin - BUG#M0000316 */
	else if (HasQuestionMark)
	{
		if (CurrentOraParamNode == NULL)
		{
			if (variables > 0)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						errmsg("bind variable does not exist")));
		}
		else if (CurrentOraParamNode->questionmark_count > variables)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					errmsg("not all variables bound")));
		else if (CurrentOraParamNode->questionmark_count < variables)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					errmsg("bind variable does not exist")));
	}
	/* End - BUG#M0000316 */
}

/*
 * dynamic sql stmt used, complain pg var
 */
void
setdynamic_haspgparam(bool value)
{
	HasPgParam = value;
}

/*
 * set dynamic DoStmt parser status
 */
void
setdynamic_doparser(bool value)
{
	IsDynDoStmt = value;
}

/*
 * set Dostmt check var status
 */
void
setdynamic_docheckvar(bool values)
{
	DoStmtCheckVar = values;
}

/* Begin - BUG#M0000316 */
/*
 * Anonymous block stmt used, complain placehold ? var
 */
void
setdynamic_hasquestionmark(bool value)
{
	HasQuestionMark = value;
}
/* End - BUG#M0000316 */

/*
 * set IsBindByName parser status
 */
void
set_bindbyname(bool value)
{
	IsBindByName = value;
}

/*
 * get DoStmt check var status
 */
bool
getdynmaic_docheckvar(void)
{
	return DoStmtCheckVar;
}

/*
 * get IsBindByName status
 */
bool
get_bindbyname(void)
{
	return IsBindByName;
}
/* End - ReqID:SRS-SQL-ANONYOUT */

/* Begin - ReqID:SRS-SQL-CALL */
/*
 * set dynamic CallStmt parser status
 */
void
setdynamic_callparser(bool value)
{
	IsDynCallStmt = value;
}
/* End - ReqID:SRS-SQL-CALL */

/* Begin - BUG#M0000316 */
/*
 * get dynamic DoStmt parser status
 */
bool
getdynamic_doparser(void)
{
	return IsDynDoStmt;
}
/* End - BUG#M0000316 */

/* Begin - BUG#M0000376 */
/*
 * set_oraparam_mode
 * If the current compiled function is an anonymous block,
 * the variable is a placeholder, such as :x,:y,
 * set the mode.
 */
void
set_oraparam_mode(char *refname, char setmode)
{
	int numparams = 0;
	int i;

	if (CurrentOraParamNode == NULL)
		return;

	numparams = CurrentOraParamNode->param_count;

	for (i = 0; i < numparams; i++)
	{
		if (CurrentOraParamNode->params[i].name != NULL &&
			strcmp(CurrentOraParamNode->params[i].name, refname) == 0)
		{
			if (setmode == PROARGMODE_OUT)
			{
				if (CurrentOraParamNode->params[i].mode == PROARGMODE_NOT_SET)
					CurrentOraParamNode->params[i].mode = PROARGMODE_OUT;
				else if (CurrentOraParamNode->params[i].mode == PROARGMODE_IN)
					CurrentOraParamNode->params[i].mode = PROARGMODE_INOUT;
			}
			else if (setmode == PROARGMODE_IN)
			{
				if (CurrentOraParamNode->params[i].mode == PROARGMODE_NOT_SET)
					CurrentOraParamNode->params[i].mode = PROARGMODE_IN;
				else if (CurrentOraParamNode->params[i].mode == PROARGMODE_OUT)
					CurrentOraParamNode->params[i].mode = PROARGMODE_INOUT;
			}
			else if (setmode == PROARGMODE_INOUT)
			{
				CurrentOraParamNode->params[i].mode = PROARGMODE_INOUT;
			}
		}
	}

	return;
}
/* End - BUG#M0000376 */
