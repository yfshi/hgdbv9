/*---------------------------------------------------------------------------------
 *
 * File:dbms_ddl.c
 *
 * Abstract:
 *		Internal implementation of DBMS_DDL package
 *
 *
 * OA-REQ-2198
 *
 * Authored by zhaoshengbin@highgo.com,20260306.
 *
 * Copyright:
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  contrib/ivorysql_ora/src/builtin_packages/dbms_ddl/dbms_ddl.c
 *
 *---------------------------------------------------------------------------------
 */

#include "postgres.h"
#include "nodes/parsenodes.h"
#include "nodes/makefuncs.h"
#include "port.h"
#include "utils/builtins.h"
#include "fmgr.h"
#include "catalog/namespace.h"
#include "utils/lsyscache.h"
#include "nodes/pg_list.h"
#include "commands/defrem.h"
#include "commands/packagecmds.h"
#include "commands/trigger.h"
#include "parser/scansup.h"
#include "utils/plsqlwrap.h"

typedef enum AlterCompileType
{
	ACT_UNKNOWN = 0,
	ACT_PROCEDURE,
	ACT_FUNCTION,
	ACT_PACKAGE,
	ACT_PACKAGE_BODY,
	ACT_TRIGGER
} AlterCompileType;

static AlterCompileType parse_alter_compile_type(char *typname);
static char *ora_normalize_identifier(char *name);

static AlterCompileType
parse_alter_compile_type(char *typname)
{
	if (pg_strcasecmp(typname, "procedure") == 0)
		return ACT_PROCEDURE;
	if (pg_strcasecmp(typname, "function") == 0)
		return ACT_FUNCTION;
	if (pg_strcasecmp(typname, "package") == 0)
		return ACT_PACKAGE;
	if (pg_strcasecmp(typname, "package body") == 0)
		return ACT_PACKAGE_BODY;
	if (pg_strcasecmp(typname, "trigger") == 0)
		return ACT_TRIGGER;

	return ACT_UNKNOWN;
}

static char *
ora_normalize_identifier(char *name)
{
	int 	len;
	char	*tmp = NULL;

	if (name == NULL)
		return NULL;

	len = strlen(name);

	/* quoted identifier */
	if (len >= 2 && name[0] == '"' && name[len-1] == '"')
	{
		tmp = pnstrdup(name + 1, len - 2);

		if (identifier_case_switch == INTERCHANGE && enable_case_switch)
			tmp = identifier_case_transform(tmp, strlen(tmp));

		return tmp;
	}

	/* unquoted identifier -> lower */
	return downcase_identifier(pstrdup(name), strlen(name), true, true);
}


PG_FUNCTION_INFO_V1(dbmsddl_alter_compile);

/*
 * dbmsddl_alter_compile
 *
 *	implement the internal dbms_ddl.alter_compile function
 */
Datum
dbmsddl_alter_compile(PG_FUNCTION_ARGS)
{
	char			*typname = NULL;
	char			*schemaname = NULL;
	char			*objname = NULL;
	AlterCompileType	type;

	/* type */
	if (!PG_ARGISNULL(0))
	{
		typname = text_to_cstring(PG_GETARG_TEXT_PP(0));
	}
	else
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("bad value for object type: ")));

	/* schema */
	if (!PG_ARGISNULL(1))
		schemaname = text_to_cstring(PG_GETARG_TEXT_PP(1));

	/* normalize identifier */
	if (schemaname)
		schemaname = ora_normalize_identifier(schemaname);

	/* name */
	if (!PG_ARGISNULL(2))
		objname = text_to_cstring(PG_GETARG_TEXT_PP(2));
	else
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("missing identifier")));

	/* normalize identifier */
	objname = ora_normalize_identifier(objname);

	/*
	 * type check
	 */
	type = parse_alter_compile_type(typname);

	switch(type)
	{
		case ACT_PROCEDURE:
			{
				CompileFunctionStmt * act_proc = makeCompileFunctionStmtNoArgs(schemaname, objname, OBJECT_PROCEDURE);
				(void)CompileFunction(act_proc);
			}
			break;
		case ACT_FUNCTION:
			{
				CompileFunctionStmt * act_func = makeCompileFunctionStmtNoArgs(schemaname, objname, OBJECT_FUNCTION);
				(void)CompileFunction(act_func);
			}
			break;
		case ACT_PACKAGE:
			{
				AlterPackageStmt *act_pkg = makeAlterPackageOrPackageBody(schemaname, objname, alter_compile_package);
				(void)AlterPackage(act_pkg);
			}
			break;
		case ACT_PACKAGE_BODY:
			{
				AlterPackageStmt *act_pkgbody = makeAlterPackageOrPackageBody(schemaname, objname, alter_compile_body);
				(void)AlterPackage(act_pkgbody);
			}
			break;
		case ACT_TRIGGER:
			{
				AlterOraTrigStmt * act_trig = makeAlterOraTrigStmtCompile(schemaname, objname, 'c');
				(void)AlterOraTrigger(act_trig);
			}
			break;

		default:
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("bad value for object type: %s", typname)));
	}
	PG_RETURN_VOID();
}


/* Begin - Reqid:SRS-PLPGSQL-HIDDEN*/
PG_FUNCTION_INFO_V1(dbms_ddl_wrap);

/*
 * SQL-callable implementation of SYS.DBMS_DDL.WRAP.
 *
 * The input is a complete PL/SQL DDL statement.  The function returns a DDL
 * statement whose body is replaced by the IvorySQL wrapped payload while the
 * object header remains readable and executable.
 */
Datum
dbms_ddl_wrap(PG_FUNCTION_ARGS)
{
	text	   *src = PG_GETARG_TEXT_PP(0);
	char	   *ddl = text_to_cstring(src);
	char	   *wrapped = ivy_plsql_wrap_ddl(ddl);

	PG_RETURN_TEXT_P(cstring_to_text(wrapped));
}
/* End - Reqid:SRS-PLPGSQL-HIDDEN*/