/*---------------------------------------------------------------------------------
*
* File:dbms_utility.c
*
* Abstract:
*		Internal implementation of DBMS_UTILITY package
*
* ReqID: SRS-PLSQL-DBMS_UTILITY
* Copyright:
* Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
*
* IDENTIFICATION
*	  contrib/ivorysql_ora/src/builtin_packages/dbms_utility/dbms_utility.c
*
*---------------------------------------------------------------------------------
*/

#include "dbms_utility.h"

#define NS_PER_CENTISECOND 10000000    // 1 centisecond = 10000000 ns
#define CENTISECOND_PER_SEC 100     // 1 second = 100 centisecond
#define ERROR_MESSAGE "ERROR MESSAGE: "

int depth = 0;
List *stack = NIL;
MemoryContext dbmsUtilityMemCtx;
int exceptionDepth = 0;

static void expand_view(RangeTblEntry *rte);
static void expand_query(Query *query);
static Query* convert_sql_to_query(const char *sqlStr);

PG_FUNCTION_INFO_V1(dbms_utility_expand_sql_text);
PG_FUNCTION_INFO_V1(dbms_utility_get_cpu_time);
PG_FUNCTION_INFO_V1(dbms_utility_get_endianness);
PG_FUNCTION_INFO_V1(dbms_utility_get_error_stack);
PG_FUNCTION_INFO_V1(dbms_utility_exec_ddl_statement);

Datum
dbms_utility_get_cpu_time(PG_FUNCTION_ARGS)
{
	int64 centisecond;
#ifdef _WIN32
	FILETIME creation, exit, kernel, user;
	ULARGE_INTEGER total;
	if (!GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user))
	{
		ereport(ERROR, (errmsg("get time failed!")));
	}
	total.LowPart = user.dwLowDateTime + kernel.dwLowDateTime;
	total.HighPart = user.dwHighDateTime + kernel.dwHighDateTime;
	centisecond = total.QuadPart / 100000;
#else
	struct timespec ts;
	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) != 0)
	{
		ereport(ERROR, (errmsg("get time failed!")));
	}
	centisecond = (int64)ts.tv_sec * CENTISECOND_PER_SEC + (int64)(ts.tv_nsec / NS_PER_CENTISECOND);
#endif
	PG_RETURN_INT64(centisecond);
}

// return true if little endianness
Datum
dbms_utility_get_endianness(PG_FUNCTION_ARGS)
{
	bool ret;
	uint16 x = 0x0001;
	ret = (*(uint8 *)&x == 1);
	PG_RETURN_BOOL(ret);
}

Datum
dbms_utility_expand_sql_text(PG_FUNCTION_ARGS)
{
	char *sqlStr = text_to_cstring(PG_GETARG_TEXT_PP(0));
	Query* query;
	char* sql = "";
	depth = 0;
	query = convert_sql_to_query(sqlStr);
	if (query != NULL)
		sql = ivy_get_query_def(query);
	PG_RETURN_VARCHAR_P(CStringGetTextDatum(sql));
}

static Query*
convert_sql_to_query(const char* sqlStr)
{
	List* parsetreeList = NULL;
	Query* query;
	RawStmt* parsetree;

	depth++;
	parsetreeList = pg_parse_query(sqlStr);
	parsetree = linitial(parsetreeList);
	Assert(list_length(parsetreeList) == 1);
	query = parse_analyze(parsetree, sqlStr, NULL, 0, NULL);
	if (depth == 1 && query->commandType != CMD_SELECT)
	{
		ereport(ERROR, (errmsg("This statement is not supported by EXPAND_SQL_TEXT, EXPAND_SQL_TEXT supports only SELECT statements")));
	}
	if (query != NULL) {
		expand_query(query);
	}
	return query;
}

static void
expand_query(Query *query)
{
	ListCell    *l1;
	if (query->rtable != NULL)
	{
		ListCell *lc;
		foreach (lc, query->rtable)
		{
			RangeTblEntry *rte = (RangeTblEntry *)lfirst(lc);
			switch (rte->rtekind)
			{
				case RTE_RELATION:
					if (rte->relkind == RELKIND_VIEW)
					{
						expand_view(rte);
					}
					break;
				case RTE_SUBQUERY:
					expand_query(rte->subquery);
					break;
				default:
					break;
			}
		}
	}

	if (query->cteList != NULL)
	{
		ListCell *l;
		CommonTableExpr *cte;
		foreach(l, query->cteList)
		{
			cte = (CommonTableExpr *) lfirst(l);
			expand_query(castNode(Query, cte->ctequery));
		}
	}

	// handle scalar subquery, like "SELECT (SELECT COUNT(*) FROM v_emp_simple) AS emp_count"
	foreach(l1, query->targetList)
	{
		TargetEntry *tle = (TargetEntry *) lfirst(l1);
		SubLink *sublink = (SubLink*)(tle->expr);
		if (sublink->subLinkType == EXPR_SUBLINK)
		{
			Query * query = (Query*)(sublink->subselect);
			expand_query(query);
		}
	}
}

static void
expand_view(RangeTblEntry *rte)
{
	Query *viewQuery;
	Datum viewdef;
	char* viewdef_p = NULL;

	viewdef = DirectFunctionCall1(pg_get_viewdef, ObjectIdGetDatum(rte->relid));
	viewdef_p = TextDatumGetCString(viewdef);

	viewQuery = convert_sql_to_query(viewdef_p);

	rte->rtekind = RTE_SUBQUERY;
	rte->subquery = viewQuery;
	rte->relkind = 0;
}

Datum
dbms_utility_get_error_stack(PG_FUNCTION_ARGS)
{
	ListCell* cell;
	StringInfoData str;
	MemoryContext oldMemCtx;
	char* retStr = "";
	int innerDepth = 0;

	if (exceptionDepth == 0)
	{
		return CStringGetTextDatum(retStr);
	}

	initStringInfo(&str);
	oldMemCtx = MemoryContextSwitchTo(dbmsUtilityMemCtx);
	foreach(cell, stack)
	{
		char* errInfo = lfirst(cell);
		appendStringInfo(&str,"%s\n", errInfo);
		if (innerDepth++ > exceptionDepth)
			break;
	}
	MemoryContextSwitchTo(oldMemCtx);

	retStr = (char*)palloc0(str.len + 1);
	memcpy(retStr, str.data, str.len);
	return CStringGetTextDatum(retStr);
}

void
collect_error_message(ErrorData* edata)
{
	char* errMsg;
	char* errStack = GetErrorContextStack();
	int size = 0;
	if (errStack != NULL)
	{
		MemoryContext oldMemCtx;
		if (dbmsUtilityMemCtx == NULL)
		{
			dbmsUtilityMemCtx = AllocSetContextCreate(TopMemoryContext,
												"dbms utility context",
												ALLOCSET_DEFAULT_SIZES);
		}
		oldMemCtx = MemoryContextSwitchTo(dbmsUtilityMemCtx);
		size = strlen(ERROR_MESSAGE) + strlen(errStack) + strlen(edata->message) + 2;
		errMsg = (char*)palloc0(size);
		memcpy(errMsg, ERROR_MESSAGE, strlen(ERROR_MESSAGE));

		memcpy(errMsg + strlen(ERROR_MESSAGE), edata->message, strlen(edata->message));
		errMsg[strlen(ERROR_MESSAGE) + strlen(edata->message)] = '\n';

		memcpy(errMsg + strlen(ERROR_MESSAGE) + strlen(edata->message) + 1, errStack, strlen(errStack));
		if (exceptionDepth == 0)
		{
			list_free(stack);
			stack = NULL;
		}
		stack = lappend(stack, errMsg);
		MemoryContextSwitchTo(oldMemCtx);
	}
}

void
collect_error_message_start(void)
{
	if (stack != NIL)
	{
		list_free(stack);
		stack = NIL;
	}
	if (dbmsUtilityMemCtx != NULL)
		MemoryContextReset(dbmsUtilityMemCtx);
	else
		dbmsUtilityMemCtx = AllocSetContextCreate(TopMemoryContext,
												"dbms utility context",
												ALLOCSET_DEFAULT_SIZES);
	exceptionDepth = 0;
}

void
collect_error_message_finish(void)
{
	if (stack != NIL)
	{
		list_free(stack);
		stack = NIL;
	}
}

void
collect_error_message_inc_depth(void)
{
	exceptionDepth += 1;
}

void
collect_error_message_dec_depth(void)
{
	exceptionDepth -= 1;
	if (stack != NULL)
	{
		stack = list_delete_last(stack);
	}
}

Datum
dbms_utility_exec_ddl_statement(PG_FUNCTION_ARGS)
{
	char *sqlStr = text_to_cstring(PG_GETARG_TEXT_PP(0));
	List* parsetreeList = NULL;
	Query* query;
	RawStmt* parsetree;
	bool ret;

	parsetreeList = pg_parse_query(sqlStr);
	parsetree = linitial(parsetreeList);
	if (list_length(parsetreeList) != 1)
	{
		ereport(ERROR, (errmsg("SQL command not properly ended")));
	}
	query = parse_analyze(parsetree, sqlStr, NULL, 0, NULL);
	if (query->commandType == CMD_UTILITY)
		ret = true;
	else
		ret = false;

	PG_RETURN_BOOL(BoolGetDatum(ret));
}
