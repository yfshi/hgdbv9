/*-------------------------------------------------------------------------
 *
 * global_session_var.c
 *	  Implenmentation of the MySQL SET global/session varibale functionality
 *
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 *
 * IDENTIFICATION
 *	  contrib/ivorysql_my/src/set/global_session_var.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "fmgr.h"
#include "nodes/parsenodes.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"
#include "utils/varlena.h"
#include "executor/spi.h"
#include "parser/scansup.h"
#include "parser/analyze.h"


PG_FUNCTION_INFO_V1(setSystemSessionVariable);
PG_FUNCTION_INFO_V1(setSystemVariable);
PG_FUNCTION_INFO_V1(setSessionVariable);
PG_FUNCTION_INFO_V1(setLocalVariable);

/* setSystemSessionVariable
 *
 * Set GUC Parameter.
 */
Datum
setSystemSessionVariable(PG_FUNCTION_ARGS)
{
	char *varName;
	char *varValue;
	VariableSetStmt *variableSetStmt;
	bool isTopLevel = true;

	if (PG_ARGISNULL(0))
		PG_RETURN_VOID();

	varName = TextDatumGetCString(PG_GETARG_DATUM(0));
	if (!PG_ARGISNULL(1))
	{
		Datum	datumValue = PG_GETARG_DATUM(1);
		Oid		varType;
		Oid		typOutput;
		bool	typIsVarlena;

		varType = get_fn_expr_argtype(fcinfo->flinfo, 1);
		getTypeOutputInfo(varType, &typOutput, &typIsVarlena);
		varValue = OidOutputFunctionCall(typOutput, datumValue);
	}
	else 
		varValue = NULL;

	variableSetStmt = makeNode(VariableSetStmt);
	variableSetStmt->name = varName;
	if (PG_GETARG_BOOL(2) == true)
	{
		variableSetStmt->kind = VAR_SET_DEFAULT;
		variableSetStmt->args = NIL;
	}
	else
	{
		variableSetStmt->kind = VAR_SET_VALUE;
		if (guc_list_input_check(varName))
		{
			List	   *namelist;
			ListCell   *cell;

			SplitIdentifierString(varValue, ',', &namelist);
			foreach(cell, namelist)
			{
				variableSetStmt->args = lappend(variableSetStmt->args, ivy_makeStringConst((char *)lfirst(cell), -1));
			}
		}
		else
		{
			variableSetStmt->args = list_make1(ivy_makeStringConst(varValue, -1));
		}
	}
	ExecSetVariableStmt(variableSetStmt, isTopLevel);

	PG_RETURN_VOID();
}

/* setSystemVariable
 *
 * Set GUC Parameter.
 */
Datum
setSystemVariable(PG_FUNCTION_ARGS)
{
	PG_RETURN_DATUM(setSystemSessionVariable(fcinfo));
}

/* setSessionVariable
 *
 * Set GUC Parameter.
 */
Datum
setSessionVariable(PG_FUNCTION_ARGS)
{
	PG_RETURN_DATUM(setSystemSessionVariable(fcinfo));
}

/* setLocalVariable
 *
 * Set GUC Parameter.
 */
Datum
setLocalVariable(PG_FUNCTION_ARGS)
{
	PG_RETURN_DATUM(setSystemSessionVariable(fcinfo));
}
