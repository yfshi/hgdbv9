/* -------------------------------------------------------------------------
 *
 * File: sql_sentinel.c
 *
 * Abstract:
 * This file is check sql length & restrict html & other unsafe sql
 *
 * Authored by tanyang@highgo.com, 20260920
 *
 * Copyright:
 * 	Copyright (c) 2009-2025, HighGo Software Co.,Ltd. All right reserved
 *
 * Identification:
 *	contrib/sql_sentinel/sql_sentinel.c
 *
 * -------------------------------------------------------------------------
 */
#include "postgres.h"

#include "fmgr.h"
#include "miscadmin.h"

#include <ctype.h>

#include <libxml/HTMLparser.h>
#include <libxml/tree.h>

#include "access/tupdesc.h"
#include "catalog/namespace.h"
#include "catalog/pg_type.h"
#include "executor/executor.h"
#include "lib/stringinfo.h"
#include "nodes/nodeFuncs.h"
#include "optimizer/optimizer.h"
#include "parser/analyze.h"
#include "parser/parse_node.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"
#include "tcop/utility.h"

#include "sql_sentinel.h"
#include "utils/varlena.h"
#include "postmaster/bgworker.h"
#include "access/genam.h"
#include "access/table.h"

#include "nodes/makefuncs.h"
#include "utils/fmgroids.h"
#include "utils/snapmgr.h"
#include "access/htup_details.h"
#include "catalog/indexing.h"
#include "catalog/objectaccess.h"
#include "catalog/pg_proc.h"
#include "utils/syscache.h"
#include "catalog/pg_namespace.h"
#include "parser/parsetree.h"


PG_MODULE_MAGIC;

void _PG_init(void);
void _PG_fini(void);

#define SQL_SECURITY_VERSION "0.1"

#define DEFAULT_SENSITIVE_FUNCTION "pg_sleep, pg_sleep_for , pg_sleep_until , sleep , benchmark ," \
				"pg_read_file , pg_read_binary_file , pg_ls_dir , pg_stat_file" \
				"lo_import , dblink_connect , dblink_exec"

#define CONSTANT_TAUTOLOGY_S	"taut_const"
#define SENSITIVE_FUNCTION_S	"sensitive_fun"
#define STACKED_QUERY_S		"stack_qry"
#define SYSTEM_CATALOG_S		"catalog"
#define SQL_UNION_S			"union"
#define COMMENT_S				"comment"
#define DEFAULT_INTERCEPT_TARGET CONSTANT_TAUTOLOGY_S "," SENSITIVE_FUNCTION_S "," STACKED_QUERY_S

#define EVENT_CONSTANT_TAUTOLOGY	(1 << 0)
#define EVENT_SENSITIVE_FUNCTION	(1 << 1)
#define EVENT_STACKED_QUERY			(1 << 2)
#define EVENT_SYSTEM_CATALOG		(1 << 3)
#define EVENT_SQL_UNION				(1 << 4)
#define EVENT_COMMENT				(1 << 5)

#define DEFAULT_1GB (1 * 1024 * 1024 * 1024)

#define SQL_SENTINEL_SENTIVE "sql_sentinel_sentive"
#define HG_SECURITY_SCHEMA "hg_security"

static bool sql_injection_enabled = false;

static char *intercept_target = NULL;
static int intercept_target_mask = 0;

static bool xss_enabled = false;
static char *xss_columns = NULL;
static char *xss_unsafe_tags = NULL;
static char *xss_unsafe_attribute = NULL;
static char *xss_unsafe_url = NULL;

static post_parse_analyze_hook_type prev_post_parse_analyze_hook = NULL;
static ExecutorCheckWrite_hook_type prev_ExecutorCheckWrite_hook = NULL;
static ProcessUtility_hook_type next_ProcessUtility_hook = NULL;
static ExecutorStart_hook_type next_ExecutorStart_hook = NULL;
static post_query_check_length_hook_type pre_post_check_sql_length = NULL;

/*
 * Keep the analyzed decision until pg_audit has created its normal event at
 * ExecutorStart/ProcessUtility.  Raising ERROR there preserves failed-SQL
 * auditing while still preventing the statement from executing.
 */
static bool sql_block_pending = false;
static char sql_pending_rules[1024];
static char *sql_pending_source = NULL;
static MemoryContext SqlSensitiveGucContext = NULL;
static int sql_sentinel_max_sql_length = DEFAULT_1GB;

typedef struct SqlRiskContext
{
	bool			risk;
	bool		has_union;
	bool		has_catalog;
	StringInfoData rules;
} SqlRiskContext;

typedef enum XssPolicy
{
	XSS_POLICY_NONE,
	XSS_POLICY_PLAIN_TEXT,
	XSS_POLICY_SAFE_HTML
} XssPolicy;

typedef struct XssScanContext
{
	XssPolicy	policy;
	const char *reason;
	xmlNodePtr	wrapper;
} XssScanContext;

typedef struct SqlSentinelRuleEntry
{
	const char *name;
	int value;
}SqlSentinelRuleEntry;

static const SqlSentinelRuleEntry intercept_target_names[] = {
	{CONSTANT_TAUTOLOGY_S, EVENT_CONSTANT_TAUTOLOGY},
	{SENSITIVE_FUNCTION_S, EVENT_SENSITIVE_FUNCTION},
	{STACKED_QUERY_S, EVENT_STACKED_QUERY},
	{SYSTEM_CATALOG_S, EVENT_SYSTEM_CATALOG},
	{SQL_UNION_S, EVENT_SQL_UNION},
	{COMMENT_S, EVENT_COMMENT},
	{NULL, 0}
};

static void sql_sentinel_post_parse_analyze(ParseState *pstate, Query *query,
											 JumbleState *jstate);
static void sql_sentinel_check_write(Relation relation, TupleTableSlot *slot,
									 CmdType operation);
static bool sql_sentinel_walker(Node *node, void *context);
static bool source_has_comment(const char *source);
static bool source_has_stacked_query(const char *source);
static const char *skip_dollar_quoted(const char *source);
static const char *skip_sql_comment(const char *source);
static void add_rule(SqlRiskContext *context, const char *rule);
static XssPolicy get_xss_policy(Relation relation, const char *column);
static bool html_is_dangerous(const char *value, XssPolicy policy,
							  const char **reason);

/*
 * Insert a row into t1 after resolving f_oid from schemaname and funcname.
 * funcname must include the argument types (e.g. "sqrt(integer)"); the OID is
 * resolved via the regprocedure input function, which raises an error when
 * the function cannot be found or is ambiguous.
 */
PG_FUNCTION_INFO_V1(sql_sentinel_insert_sentive);
PG_FUNCTION_INFO_V1(sql_sentinel_delete_sentive);


Datum
sql_sentinel_insert_sentive(PG_FUNCTION_ARGS)
{
	text		 *schemaname = PG_GETARG_TEXT_PP(0);
	text		 *funcname = PG_GETARG_TEXT_PP(1);
	char		 *qualified;
	Oid					  func_oid;
	RangeVar	 *rv;
	Oid					  relid;
	Relation		  rel;
	TupleDesc 	  tupdesc;
	Datum 		  values[3];
	bool			  nulls[3];
	HeapTuple 	  tuple;

	qualified = psprintf("%s.%s",
						   text_to_cstring(schemaname),
						   text_to_cstring(funcname));

	func_oid = DatumGetObjectId(
		  OidFunctionCall1(F_REGPROCEDUREIN, CStringGetDatum(qualified)));

	rv = makeRangeVar(HG_SECURITY_SCHEMA, SQL_SENTINEL_SENTIVE, -1);
	relid = RangeVarGetRelid(rv, RowExclusiveLock, false);
	rel = table_open(relid, RowExclusiveLock);

	tupdesc = RelationGetDescr(rel);

	memset(values, 0, sizeof(values));
	memset(nulls, false, sizeof(nulls));

	values[0] = PointerGetDatum(schemaname);
	values[1] = PointerGetDatum(funcname);
	values[2] = ObjectIdGetDatum(func_oid);

	tuple = heap_form_tuple(tupdesc, values, nulls);

	CatalogTupleInsert(rel, tuple);

	heap_freetuple(tuple);
	table_close(rel, RowExclusiveLock);

	PG_RETURN_VOID();
}

/*
 * Delete the row of t1 whose schemaname and funcname match the given
 * strings, locating it through the unique index on (schemaname, funcname).
 */
static void
sql_sentinel_delete_from_sentive(const char *schemaname, const char *funcname,
										bool missing_ok)
{
	RangeVar   *rv;
	Oid                     relid;
	Relation        rel;
	SysScanDesc scan;
	ScanKeyData key[2];
	HeapTuple       tuple;

	rv = makeRangeVar(HG_SECURITY_SCHEMA, SQL_SENTINEL_SENTIVE, -1);
	relid = RangeVarGetRelid(rv, RowExclusiveLock, missing_ok);
	if (!OidIsValid(relid))
		return;

	rel = table_open(relid, RowExclusiveLock);

	ScanKeyInit(&key[0],
			1,
			BTEqualStrategyNumber,
			F_TEXTEQ,
			CStringGetTextDatum(schemaname));
	ScanKeyInit(&key[1],
			2,
			BTEqualStrategyNumber,
			F_TEXTEQ,
			CStringGetTextDatum(funcname));

	scan = systable_beginscan(rel, InvalidOid, true,
				NULL, 2, key);

	if (HeapTupleIsValid(tuple = systable_getnext(scan)))
		CatalogTupleDelete(rel, &tuple->t_self);

	systable_endscan(scan);
	table_close(rel, RowExclusiveLock);
}

static object_access_hook_type prev_object_access_hook = NULL;


Datum
sql_sentinel_delete_sentive(PG_FUNCTION_ARGS)
{
	text       *schemaname = PG_GETARG_TEXT_PP(0);
	text       *funcname = PG_GETARG_TEXT_PP(1);

	sql_sentinel_delete_from_sentive(text_to_cstring(schemaname),
									text_to_cstring(funcname),
									false);

	PG_RETURN_VOID();
}


/*
 * Object access hook: when a function is dropped, remove the matching row
 * from t1 automatically.
 */
static void
sql_sentinel_object_access_hook(ObjectAccessType access, Oid classId,
																Oid objectId, int subId, void *arg)
{
	HeapTuple		proctup;
	Form_pg_proc procform;
	char	   *schemaname;
	char	   *proname;
	StringInfoData buf;
	int 					nargs;
	int 					i;



	if (prev_object_access_hook)
			prev_object_access_hook(access, classId, objectId, subId, arg);

	if (access != OAT_DROP || classId != ProcedureRelationId || subId != 0)
			return;

	proctup = SearchSysCache1(PROCOID, ObjectIdGetDatum(objectId));
	if (!HeapTupleIsValid(proctup))
			return;

	procform = (Form_pg_proc) GETSTRUCT(proctup);

	schemaname = get_namespace_name(procform->pronamespace);
	proname = NameStr(procform->proname);
	nargs = procform->pronargs;

	initStringInfo(&buf);
	appendStringInfo(&buf, "%s(", proname);
	for (i = 0; i < nargs; i++)
	{
			if (i > 0)
					appendStringInfoChar(&buf, ',');
			appendStringInfoString(&buf,
													   format_type_be(procform->proargtypes.values[i]));
	}
	appendStringInfoChar(&buf, ')');

	sql_sentinel_delete_from_sentive(schemaname, buf.data, true);

	pfree(buf.data);
	ReleaseSysCache(proctup);
}



static void
sql_sentinel_check_sql_length(const char *sql_string)
{
	if (sql_string != NULL &&
		strlen(sql_string) > (size_t)sql_sentinel_max_sql_length)
	{
		ereport(ERROR, (errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
						errmsg("SQL statement is too long (maximum %d bytes)", sql_sentinel_max_sql_length)));
	}
}

static void
remember_sql_block(const char *source, int risk, const char *rules)
{
	MemoryContext old_context;

	if (sql_pending_source != NULL)
		pfree(sql_pending_source);

	old_context = MemoryContextSwitchTo(TopMemoryContext);
	sql_pending_source = pstrdup(source != NULL ? source : "");
	MemoryContextSwitchTo(old_context);

	strlcpy(sql_pending_rules, rules, sizeof(sql_pending_rules));
	sql_block_pending = true;
}

void
sql_sentinel_block_pending(const char *query_string)
{
	char rules[sizeof(sql_pending_rules)];

	if (!sql_block_pending || query_string == NULL ||
		sql_pending_source == NULL ||
		strcmp(sql_pending_source, query_string) != 0)
		return;

	strlcpy(rules, sql_pending_rules, sizeof(rules));
	sql_block_pending = false;
	pfree(sql_pending_source);
	sql_pending_source = NULL;

	ereport(ERROR,
			(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
			 errmsg("SQL injection attack detected: rules=%s",
					 rules)));
}

static void
add_rule(SqlRiskContext *context, const char *rule/*, int risk*/)
{
	if (context->rules.len > 0)
		appendStringInfoChar(&context->rules, ',');
	appendStringInfoString(&context->rules, rule);
	context->risk = true;
}

static bool
constant_true(Node *node)
{
	Node *folded;

	/* Use PostgreSQL's resolved operators and immutable constant folding. */
	folded = eval_const_expressions(NULL, node);
	return IsA(folded, Const) &&
		((Const *) folded)->consttype == BOOLOID &&
		!((Const *) folded)->constisnull &&
		DatumGetBool(((Const *) folded)->constvalue);
}

static bool
setop_has_union(Node *node)
{
	SetOperationStmt *setop;

	if (node == NULL || IsA(node, RangeTblRef))
		return false;
	if (!IsA(node, SetOperationStmt))
		return false;

	setop = (SetOperationStmt *) node;
	return setop->op == SETOP_UNION ||
		setop_has_union(setop->larg) || setop_has_union(setop->rarg);
}

static void
inspect_query(Query *query, SqlRiskContext *context)
{
	ListCell *lc;

	if ((intercept_target_mask & EVENT_SQL_UNION) && setop_has_union(query->setOperations))
		context->has_union = true;

	if ((intercept_target_mask & EVENT_SYSTEM_CATALOG))
	{
		foreach(lc, query->rtable)
		{
			RangeTblEntry *rte = lfirst_node(RangeTblEntry, lc);

			if (rte->rtekind == RTE_RELATION)
			{
				Oid namespace_oid = get_rel_namespace(rte->relid);
				char *namespace_name = get_namespace_name(namespace_oid);
				char *relation_name = get_rel_name(rte->relid);

				if ((namespace_name != NULL &&
					 (pg_strcasecmp(namespace_name, "pg_catalog") == 0 ||
					  pg_strcasecmp(namespace_name, "information_schema") == 0 ||
					  pg_strcasecmp(namespace_name, "hg_security") == 0)) ||
					(relation_name != NULL && strncmp(relation_name, "pg_", 3) == 0))
					context->has_catalog = true;
			}
		}
	}
}

static bool
sql_sentinel_is_sensitive(Oid funcid)
{
	RangeVar *rv = NULL;
	Oid  relid = InvalidOid;
	Relation rel;
	SysScanDesc scan;
	ScanKeyData key[1];
	HeapTuple tuple;
	bool found = false;

	rv = makeRangeVar(HG_SECURITY_SCHEMA, SQL_SENTINEL_SENTIVE, -1);
	relid = RangeVarGetRelid(rv, AccessShareLock, true);
	if (!OidIsValid(relid))
		return false;

	rel = table_open(relid, AccessShareLock);

	ScanKeyInit(&key[0], 3, BTEqualStrategyNumber, F_OIDEQ, ObjectIdGetDatum(funcid));

	scan = systable_beginscan(rel, InvalidOid, true, NULL, 1, key);

	tuple = systable_getnext(scan);

	found = HeapTupleIsValid(tuple);
	systable_endscan(scan);
	table_close(rel, AccessShareLock);

	return found;
}

static bool
sql_sentinel_walker(Node *node, void *context_arg)
{
	SqlRiskContext *context = (SqlRiskContext *) context_arg;

	if (node == NULL)
		return false;

	if (IsA(node, Query))
	{
		inspect_query((Query *) node, context);
		return query_tree_walker((Query *) node, sql_sentinel_walker,
								 context, 0);
	}

	if (IsA(node, BoolExpr))
	{
		BoolExpr *boolean = (BoolExpr *) node;
		ListCell *lc;

		if (boolean->boolop == OR_EXPR && (intercept_target_mask & EVENT_CONSTANT_TAUTOLOGY))
		{
			foreach(lc, boolean->args)
			{
				if (constant_true((Node *) lfirst(lc)))
				{
					add_rule(context, CONSTANT_TAUTOLOGY_S);
					break;
				}
			}
		}
	}
	else if (IsA(node, FuncExpr))
	{
		FuncExpr *function = (FuncExpr *) node;

		if (intercept_target_mask & EVENT_SENSITIVE_FUNCTION)
		{
			if (sql_sentinel_is_sensitive(function->funcid))
			{
				add_rule(context, SENSITIVE_FUNCTION_S);
			}
		}
	}

	return expression_tree_walker(node, sql_sentinel_walker, context);
}

/* Return the first byte after a complete dollar-quoted body, if present. */
static const char *
skip_dollar_quoted(const char *source)
{
	const char *tag_end;
	const char *body;
	size_t delimiter_length;

	if (*source != '$')
		return NULL;
	tag_end = source + 1;
	if (*tag_end != '$')
	{
		if (!isalpha((unsigned char) *tag_end) && *tag_end != '_')
			return NULL;
		while (isalnum((unsigned char) *tag_end) || *tag_end == '_')
			tag_end++;
	}
	if (*tag_end != '$')
		return NULL;

	delimiter_length = (size_t) (tag_end - source + 1);
	for (body = tag_end + 1; *body != '\0'; body++)
	{
		if (*body == '$' &&
			strncmp(body, source, delimiter_length) == 0)
			return body + delimiter_length;
	}
	return NULL;
}

/* Return the first byte after a SQL comment, or NULL if this is not one. */
static const char *
skip_sql_comment(const char *source)
{
	if (source[0] == '-' && source[1] == '-')
	{
		source += 2;
		while (*source != '\0' && *source != '\n' && *source != '\r')
			source++;
		return source;
	}

	if (source[0] == '/' && source[1] == '*')
	{
		int depth = 1;

		source += 2;
		while (*source != '\0' && depth > 0)
		{
			if (source[0] == '/' && source[1] == '*')
			{
				depth++;
				source += 2;
			}
			else if (source[0] == '*' && source[1] == '/')
			{
				depth--;
				source += 2;
			}
			else
				source++;
		}
		return source;
	}

	return NULL;
}

/* Detect comments outside quoted strings and dollar-quoted bodies. */
static bool
source_has_comment(const char *source)
{
	const char *p = source;
	char quote = '\0';

	while (*p)
	{
		if (quote != '\0')
		{
			if (*p == quote)
			{
				if (p[1] == quote)
					p++;
				else
					quote = '\0';
			}
			p++;
			continue;
		}

		if (*p == '\'' || *p == '"')
		{
			quote = *p++;
			continue;
		}

		if (*p == '$')
		{
			const char *after_dollar = skip_dollar_quoted(p);

			if (after_dollar != NULL)
			{
				p = after_dollar;
				continue;
			}
		}

		if ((p[0] == '-' && p[1] == '-') ||
			(p[0] == '/' && p[1] == '*'))
			return true;
		p++;
	}
	return false;
}

/* Version 0.1: count separators outside strings, comments and dollar quotes. */
static bool
source_has_stacked_query(const char *source)
{
	const char *p = source;
	char quote = '\0';
	bool saw_separator = false;

	while (*p)
	{
		if (quote != '\0')
		{
			if (*p == quote)
			{
				if (p[1] == quote)
					p++;
				else
					quote = '\0';
			}
			p++;
			continue;
		}

		if (*p == '\'' || *p == '"')
		{
			quote = *p++;
			continue;
		}

		if (*p == '$')
		{
			const char *after_dollar = skip_dollar_quoted(p);

			if (after_dollar != NULL)
			{
				p = after_dollar;
				continue;
			}
		}

		if ((p[0] == '-' && p[1] == '-') ||
			(p[0] == '/' && p[1] == '*'))
		{
			p = skip_sql_comment(p);
			continue;
		}

		if (*p == ';')
		{
			saw_separator = true;
			p++;
			continue;
		}

		if (saw_separator && !isspace((unsigned char) *p))
			return true;
		p++;
	}
	return false;
}

/*
 * Reject direct INSERT or UPDATE on t1 so that the table can only be modified
 * through the extension's own functions.
 */
static void
sql_sentinel_check_sentive_dml(Query *query)
{
	Oid  relid;
	RangeTblEntry *rte;

	if (query->commandType != CMD_INSERT && query->commandType != CMD_UPDATE)
		return;

	if (query->resultRelation == 0)
		return;

	rte = rt_fetch(query->resultRelation, query->rtable);
	if (rte->rtekind != RTE_RELATION)
		return;

	relid = get_relname_relid(SQL_SENTINEL_SENTIVE, SECURITY_SCHEMA_NAMESPACE);
	if (!OidIsValid(relid) || rte->relid != relid)
		return;

	ereport(ERROR,
		(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
		errmsg("direct INSERT or UPDATE on sql_sentinel_sentive is not allowed")));
}


static void
sql_sentinel_post_parse_analyze(ParseState *pstate, Query *query,
								JumbleState *jstate)
{
	SqlRiskContext context;
	const char *source;

	if (prev_post_parse_analyze_hook)
		prev_post_parse_analyze_hook(pstate, query, jstate);

	if (query == NULL)
		return;

	sql_sentinel_check_sentive_dml(query);

	if (!sql_injection_enabled)
		return;

	if (MyBgworkerEntry != NULL && MyBgworkerEntry->bgw_name != NULL &&
		strncmp(MyBgworkerEntry->bgw_name, "pg_audit launcher", sizeof("pg_audit launcher")) == 0)
	{
		return;
	}

	MemSet(&context, 0, sizeof(context));
	initStringInfo(&context.rules);
	source = pstate != NULL ? pstate->p_sourcetext : NULL;

	inspect_query(query, &context);
	(void) query_tree_walker(query, sql_sentinel_walker, &context, 0);

	if (context.has_union)
		add_rule(&context, SQL_UNION_S);
	if (context.has_catalog)
		add_rule(&context, SYSTEM_CATALOG_S);
	if (source != NULL && (intercept_target_mask & EVENT_STACKED_QUERY)
			&& source_has_stacked_query(source))
		add_rule(&context, STACKED_QUERY_S);
	if (source != NULL && (intercept_target_mask & EVENT_COMMENT)
			&& source_has_comment(source))
		add_rule(&context, COMMENT_S);

	if (context.risk)
		remember_sql_block(source, context.risk, context.rules.data);

	pfree(context.rules.data);
}

static bool
token_list_contains(const char *list, const char *token)
{
	const char *p = list;
	size_t token_len = strlen(token);

	while (p != NULL && *p != '\0')
	{
		const char *end = strchr(p, ',');
		const char *start = p;
		size_t len;

		while (*start && isspace((unsigned char) *start))
			start++;
		len = end != NULL ? (size_t) (end - start) : strlen(start);
		while (len > 0 && isspace((unsigned char) start[len - 1]))
			len--;
		if (len == token_len && pg_strncasecmp(start, token, len) == 0)
			return true;
		p = end != NULL ? end + 1 : NULL;
	}
	return false;
}

static XssPolicy
get_xss_policy(Relation relation, const char *column)
{
	char *copy;
	char *entry;
	char *saveptr = NULL;
	const char *schema = get_namespace_name(RelationGetNamespace(relation));
	const char *table = RelationGetRelationName(relation);
	XssPolicy result = XSS_POLICY_NONE;

	if (xss_columns == NULL || xss_columns[0] == '\0')
		return XSS_POLICY_NONE;

	copy = pstrdup(xss_columns);
#ifdef _WIN32
	for (entry = strtok_s(copy, ",", &saveptr);
		 entry != NULL;
		 entry = strtok_s(NULL, ",", &saveptr))
#else
	for (entry = strtok_r(copy, ",", &saveptr);
		 entry != NULL;
		 entry = strtok_r(NULL, ",", &saveptr))
#endif
	{
		char *equals = strchr(entry, '=');
		char *first_dot;
		char *second_dot;
		char *policy;

		if (equals == NULL)
			continue;
		*equals = '\0';
		policy = equals + 1;
		first_dot = strchr(entry, '.');
		if (first_dot == NULL)
			continue;
		*first_dot++ = '\0';
		second_dot = strchr(first_dot, '.');
		if (second_dot == NULL)
			continue;
		*second_dot++ = '\0';

		if ((strcmp(entry, "*") == 0 || pg_strcasecmp(entry, schema) == 0) &&
			pg_strcasecmp(first_dot, table) == 0 &&
			pg_strcasecmp(second_dot, column) == 0)
		{
			while (*policy && isspace((unsigned char) *policy))
				policy++;
			if (pg_strcasecmp(policy, "plain_text") == 0)
				result = XSS_POLICY_PLAIN_TEXT;
			else if (pg_strcasecmp(policy, "safe_html") == 0)
				result = XSS_POLICY_SAFE_HTML;
			break;
		}
	}
	pfree(copy);
	return result;
}

static bool
dangerous_url(const xmlChar *value)
{
	const unsigned char *p = value;
	char normalized[32];
	int length = 0;

	while (*p && (*p <= 0x20 || *p == 0x7f))
		p++;

	while (*p && *p != ':' && length < (int) sizeof(normalized) - 2)
	{
		if (*p > 0x20 && *p != 0x7f)
			normalized[length++] = pg_tolower(*p);
		p++;
	}

	if (*p == ':')
		normalized[length++] = ':';
	normalized[length] = '\0';

	return strcmp(normalized, "javascript:") == 0 ||
		strcmp(normalized, "vbscript:") == 0 ||
		strcmp(normalized, "data:") == 0;
}

static bool
scan_html_node(xmlDocPtr document, xmlNodePtr node, XssScanContext *context)
{
	for (; node != NULL; node = node->next)
	{
		/* the node is html node */
		if (node->type == XML_ELEMENT_NODE)
		{
			const char *tag = (const char *) node->name;
			xmlAttrPtr attribute;

			if (node != context->wrapper &&
				pg_strcasecmp(tag, "html") != 0 && pg_strcasecmp(tag, "body") != 0)
			{
				/* node tag is div, script .... will coming */
				/* WHEN SET PLAIN TEXT, sentinel it */
				if (context->policy == XSS_POLICY_PLAIN_TEXT)
				{
					context->reason = "html markup is not allowed";
					return true;
				}

				/* when set xss_safe_tags the tag is safe */
				if (token_list_contains(xss_unsafe_tags, tag))
				{
					context->reason = "HTML tag is in the unsafe list";
					return true;
				}
			}

			for (attribute = node->properties;
				 node != context->wrapper && attribute != NULL;
				 attribute = attribute->next)
			{
				const char *name = (const char *) attribute->name;
				xmlChar *value;

				/* when set xss_safe_tags the tag is safe */
				if (token_list_contains(xss_unsafe_attribute, name))
				{
					context->reason = "event or style attribute is not allowed";
					return true;
				}

				/* check  attribute have URL? */
				if (!token_list_contains(xss_unsafe_url, name))
					continue;

				value = xmlNodeListGetString(document, attribute->children, 1);
				if (value != NULL && dangerous_url(value))
				{
					xmlFree(value);
					context->reason = "dangerous URL scheme is not allowed";
					return true;
				}
				if (value != NULL)
					xmlFree(value);
			}
		}

		if (scan_html_node(document, node->children, context))
			return true;
	}
	return false;
}

static xmlNodePtr
find_html_wrapper(xmlNodePtr node)
{
	for (; node != NULL; node = node->next)
	{
		xmlChar *marker;
		xmlNodePtr found;

		if (node->type == XML_ELEMENT_NODE)
		{
			marker = xmlGetProp(node, BAD_CAST "data-pg-audit-root");
			if (marker != NULL)
			{
				xmlFree(marker);
				return node;
			}
		}

		found = find_html_wrapper(node->children);
		if (found != NULL)
			return found;
	}
	return NULL;
}

static bool
html_is_dangerous(const char *value, XssPolicy policy, const char **reason)
{
	htmlDocPtr document;
	XssScanContext context;
	StringInfoData wrapped;
	bool dangerous;

	context.policy = policy;
	context.reason = NULL;
	context.wrapper = NULL;
	initStringInfo(&wrapped);
	/* build html, libxml2 parser it */
	appendStringInfoString(&wrapped, "<div data-pg-audit-root=\"1\">");
	appendStringInfoString(&wrapped, value);
	appendStringInfoString(&wrapped, "</div>");
	document = htmlReadMemory(wrapped.data, wrapped.len, NULL, "UTF-8",
						  HTML_PARSE_RECOVER | HTML_PARSE_NOERROR |
						  HTML_PARSE_NOWARNING | HTML_PARSE_NONET |
						  HTML_PARSE_NOIMPLIED | HTML_PARSE_NODEFDTD);
	pfree(wrapped.data);

	if (document == NULL)
	{
		*reason = "malformed HTML content";
		return true;
	}

	context.wrapper = find_html_wrapper(xmlDocGetRootElement(document));
	if (context.wrapper == NULL)
	{
		xmlFreeDoc(document);
		*reason = "malformed HTML content";
		return true;
	}

	dangerous = scan_html_node(document, xmlDocGetRootElement(document), &context);
	xmlFreeDoc(document);
	*reason = context.reason;

	return dangerous;
}

static void
sql_sentinel_check_write(Relation relation, TupleTableSlot *slot,
						 CmdType operation)
{
	TupleDesc tuple_desc;
	int attribute_number;

	if (prev_ExecutorCheckWrite_hook)
		prev_ExecutorCheckWrite_hook(relation, slot, operation);

	if (!xss_enabled || relation == NULL || slot == NULL)
		return;

	if (MyBgworkerEntry != NULL && MyBgworkerEntry->bgw_name != NULL &&
		strncmp(MyBgworkerEntry->bgw_name, "pg_audit launcher", sizeof("pg_audit launcher")) == 0)
	{
		return;
	}

	tuple_desc = RelationGetDescr(relation);
	for (attribute_number = 1; attribute_number <= tuple_desc->natts; attribute_number++)
	{
		Form_pg_attribute attribute = TupleDescAttr(tuple_desc, attribute_number - 1);
		XssPolicy policy;
		Datum value;
		bool is_null;
		char type_category;
		bool preferred;
		Oid output_function;
		bool variable_length;
		char *text_value;
		const char *reason = NULL;

		if (attribute->attisdropped)
			continue;

		policy = get_xss_policy(relation, NameStr(attribute->attname));
		if (policy == XSS_POLICY_NONE)
			continue;

		/* XSS only saved in TYPCATEGORY_STRING */
		get_type_category_preferred(attribute->atttypid, &type_category, &preferred);
		if (type_category != TYPCATEGORY_STRING)
			continue;

		value = slot_getattr(slot, attribute_number, &is_null);
		if (is_null)
			continue;

		getTypeOutputInfo(attribute->atttypid, &output_function, &variable_length);
		text_value = OidOutputFunctionCall(output_function, value);

		if (html_is_dangerous(text_value, policy, &reason))
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					 errmsg("XSS content detected for %s.%s: %s",
							RelationGetRelationName(relation),
							NameStr(attribute->attname), reason)));
		}
		pfree(text_value);
	}
}

static bool
intercept_target_check_hook(char **newvalue, void **extra, GucSource source)
{
	char *rawstring = NULL;
	List *elemlist = NULL;
	MemoryContext oldContext;
	ListCell *l;
	int target_mask = 0;

	if (*newvalue == NULL || *newvalue[0] == '\0')
		return true;

	/* Need a modifiable copy of string */
	rawstring = pstrdup(*newvalue);

	oldContext = MemoryContextSwitchTo(SqlSensitiveGucContext);

	/* Parse string into list of filename paths */
	if (!SplitDirectoriesString(rawstring, ',', &elemlist))
	{
		/* syntax error in list */
		pfree(rawstring);
		GUC_check_errdetail("invalid list syntax in parameter sql_sentinel.intercept_target= %s:", *newvalue);
		MemoryContextSwitchTo(oldContext);
		return false;
	}

	foreach(l, elemlist)
	{
		char *tok = (char *)lfirst(l);
		bool found = false;
		int i = 0;

		for (i = 0; intercept_target_names[i].name != NULL; i++)
		{
			if (pg_strcasecmp(tok, intercept_target_names[i].name) == 0)
			{
				target_mask |= intercept_target_names[i].value;
				found = true;
				break;
			}
		}

		if (!found)
		{
			GUC_check_errdetail("Unrecognized key word= %s:", tok);
			pfree(rawstring);
			list_free_deep(elemlist);
			return false;
		}
	}

	pfree(rawstring);
	MemoryContextSwitchTo(oldContext);
	intercept_target_mask = target_mask;

	return true;
}

void
sql_sentinel_init(void)
{
	MemoryContext	old_context;

	SqlSensitiveGucContext = AllocSetContextCreate(TopMemoryContext,
									  "sql_sensitive_guc",
									  ALLOCSET_DEFAULT_SIZES);
	old_context = MemoryContextSwitchTo(SqlSensitiveGucContext);

	DefineCustomBoolVariable("sql_sentinel.sql_injection",
							"Detect and block SQL injection attack features.",
							NULL, &sql_injection_enabled, false,
							PGC_SUSET, 0, NULL, NULL, NULL);

	DefineCustomStringVariable("sql_sentinel.intercept_target",
							"intercept target event",
							NULL, &intercept_target, DEFAULT_INTERCEPT_TARGET, PGC_SUSET, 0,
							intercept_target_check_hook, NULL, NULL);

	DefineCustomBoolVariable("sql_sentinel.xss_detection",
							"Detect and block XSS content on configured columns.",
							NULL, &xss_enabled, false,
							PGC_SUSET, 0, NULL, NULL, NULL);

	DefineCustomStringVariable("sql_sentinel.xss_columns",
							"Comma-separated schema.table.column=plain_text|safe_html policies.",
							NULL, &xss_columns, "", PGC_SUSET, 0,
							NULL, NULL, NULL);

	DefineCustomStringVariable("sql_sentinel.xss_unsafe_tags",
							"Comma-separated HTML tags allowed by safe_html policies.",
							NULL, &xss_unsafe_tags,
							"",
							PGC_SUSET, 0, NULL, NULL, NULL);

	DefineCustomStringVariable("sql_sentinel.xss_unsafe_attribute",
							"Comma-separated HTML tags allowed by safe_html policies.",
							NULL, &xss_unsafe_attribute,
							"onclick,onerror,style",
							PGC_SUSET, 0, NULL, NULL, NULL);

	DefineCustomStringVariable("sql_sentinel.xss_unsafe_url",
							"Comma-separated HTML tags allowed by safe_html policies.",
							NULL, &xss_unsafe_url,
							"href,src,action,formaction,poster",
							PGC_SUSET, 0, NULL, NULL, NULL);

	DefineCustomIntVariable("sql_sentinel.max_sql_length",
							"sets the maximum length of SQL statements accepted from clients",
							NULL,
							&sql_sentinel_max_sql_length,
							DEFAULT_1GB,
							100, DEFAULT_1GB,
							PGC_SUSET,
							GUC_UNIT_BYTE,
							NULL,
							NULL,
							NULL);

	MemoryContextSwitchTo(old_context);
}

void
sql_sentinel_fini(void)
{
	if (post_parse_analyze_hook == sql_sentinel_post_parse_analyze)
		post_parse_analyze_hook = prev_post_parse_analyze_hook;
	if (ExecutorCheckWrite_hook == sql_sentinel_check_write)
		ExecutorCheckWrite_hook = prev_ExecutorCheckWrite_hook;
	if (sql_pending_source != NULL)
	{
		pfree(sql_pending_source);
		sql_pending_source = NULL;
	}
	sql_block_pending = false;
}

static void
sql_sentinel_ProcessUtility_hook(PlannedStmt *pstmt,
										const char *queryString,
										bool readOnlyTree,
										ProcessUtilityContext context,
										ParamListInfo params,
										QueryEnvironment *queryEnv,
										DestReceiver *dest,
										QueryCompletion *completionTag)
{
	/* Block before standard_ProcessUtility performs any operation. */
	sql_sentinel_block_pending(queryString);

	/* Call the standard process utility chain. */
	if (next_ProcessUtility_hook)
		(*next_ProcessUtility_hook) (pstmt, queryString, readOnlyTree,context, params,
									queryEnv, dest, completionTag);
	else
		standard_ProcessUtility(pstmt, queryString, readOnlyTree, context, params,
								queryEnv, dest, completionTag);

}

static void
sql_sentinel_ExecutorStart_hook(QueryDesc *queryDesc, int eflags)
{
	/* Call the previous hook or standard function */
	if (next_ExecutorStart_hook)
		next_ExecutorStart_hook(queryDesc, eflags);
	else
		standard_ExecutorStart(queryDesc, eflags);

	/*
	 * Executor initialization is complete, but ExecutorRun has not started.
	 * An ERROR here follows pg_audit's normal failed-statement cleanup path.
	 */
	sql_sentinel_block_pending(queryDesc->sourceText);
}


void
_PG_init(void)
{
	sql_sentinel_init();

	prev_post_parse_analyze_hook = post_parse_analyze_hook;
	post_parse_analyze_hook = sql_sentinel_post_parse_analyze;

	prev_ExecutorCheckWrite_hook = ExecutorCheckWrite_hook;
	ExecutorCheckWrite_hook = sql_sentinel_check_write;

	next_ProcessUtility_hook = ProcessUtility_hook;
	ProcessUtility_hook = sql_sentinel_ProcessUtility_hook;

	next_ExecutorStart_hook = ExecutorStart_hook;
	ExecutorStart_hook = sql_sentinel_ExecutorStart_hook;

	pre_post_check_sql_length = post_check_sql_length;
	post_check_sql_length = sql_sentinel_check_sql_length;

	prev_object_access_hook = object_access_hook;
	object_access_hook = sql_sentinel_object_access_hook;

}

/* Uninstall hooks */
void
_PG_fini(void)
{
	sql_sentinel_fini();
}

