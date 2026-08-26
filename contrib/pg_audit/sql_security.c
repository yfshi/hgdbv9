/*-------------------------------------------------------------------------
 *
 * sql_security.c
 *    Version 0.1 SQL injection and XSS detection integrated with pg_audit.
 *
 * SQL injection rules inspect the analyzed Query tree.  XSS rules inspect
 * the final tuple immediately before INSERT/UPDATE through a core hook.
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

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

#include "sql_security.h"

#define SQL_SECURITY_VERSION "0.1"

static bool sql_injection_enabled = false;
static int sql_injection_threshold = 60;
static bool xss_enabled = false;
static char *xss_columns = NULL;
static char *xss_safe_tags = NULL;

static post_parse_analyze_hook_type prev_post_parse_analyze_hook = NULL;
static ExecutorCheckWrite_hook_type prev_ExecutorCheckWrite_hook = NULL;

/*
 * Keep the analyzed decision until pg_audit has created its normal event at
 * ExecutorStart/ProcessUtility.  Raising ERROR there preserves failed-SQL
 * auditing while still preventing the statement from executing.
 */
static bool sql_block_pending = false;
static int sql_pending_risk = 0;
static char sql_pending_rules[256];
static char *sql_pending_source = NULL;

typedef struct SqlRiskContext
{
	int			risk;
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

static void sql_security_post_parse_analyze(ParseState *pstate, Query *query,
											 JumbleState *jstate);
static void sql_security_check_write(Relation relation, TupleTableSlot *slot,
									 CmdType operation);
static bool sql_security_walker(Node *node, void *context);
static bool source_has_comment(const char *source);
static bool source_has_stacked_query(const char *source);
static const char *skip_dollar_quoted(const char *source);
static const char *skip_sql_comment(const char *source);
static void add_rule(SqlRiskContext *context, const char *rule, int risk);
static XssPolicy get_xss_policy(Relation relation, const char *column);
static bool html_is_dangerous(const char *value, XssPolicy policy,
							  const char **reason);

static void
remember_sql_block(const char *source, int risk, const char *rules)
{
	MemoryContext old_context;

	if (sql_pending_source != NULL)
		pfree(sql_pending_source);

	old_context = MemoryContextSwitchTo(TopMemoryContext);
	sql_pending_source = pstrdup(source != NULL ? source : "");
	MemoryContextSwitchTo(old_context);

	sql_pending_risk = risk;
	strlcpy(sql_pending_rules, rules, sizeof(sql_pending_rules));
	sql_block_pending = true;
}

void
audit_sql_security_block_pending(const char *query_string)
{
	int risk;
	char rules[sizeof(sql_pending_rules)];

	if (!sql_block_pending || query_string == NULL ||
		sql_pending_source == NULL ||
		strcmp(sql_pending_source, query_string) != 0)
		return;

	risk = sql_pending_risk;
	strlcpy(rules, sql_pending_rules, sizeof(rules));
	sql_block_pending = false;
	pfree(sql_pending_source);
	sql_pending_source = NULL;

	ereport(ERROR,
			(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
			 errmsg("SQL injection attack detected: risk=%d rules=%s",
					risk, rules)));
}

static void
add_rule(SqlRiskContext *context, const char *rule, int risk)
{
	if (context->rules.len > 0)
		appendStringInfoChar(&context->rules, ',');
	appendStringInfoString(&context->rules, rule);
	context->risk += risk;
}

static bool
is_named_function(Oid function_oid, const char *const *names)
{
	char *name = get_func_name(function_oid);
	int i;

	if (name == NULL)
		return false;
	for (i = 0; names[i] != NULL; i++)
	{
		if (pg_strcasecmp(name, names[i]) == 0)
		{
			pfree(name);
			return true;
		}
	}
	pfree(name);
	return false;
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

	if (setop_has_union(query->setOperations))
		context->has_union = true;

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
				  pg_strcasecmp(namespace_name, "information_schema") == 0)) ||
				(relation_name != NULL && strncmp(relation_name, "pg_", 3) == 0))
				context->has_catalog = true;
		}
	}
}

static bool
sql_security_walker(Node *node, void *context_arg)
{
	SqlRiskContext *context = (SqlRiskContext *) context_arg;
	static const char *const time_functions[] = {
		"pg_sleep", "pg_sleep_for", "pg_sleep_until", "sleep", "benchmark", NULL
	};
	static const char *const sensitive_functions[] = {
		"pg_read_file", "pg_read_binary_file", "pg_ls_dir", "pg_stat_file",
		"lo_import", "dblink_connect", "dblink_exec", NULL
	};

	if (node == NULL)
		return false;

	if (IsA(node, Query))
	{
		inspect_query((Query *) node, context);
		return query_tree_walker((Query *) node, sql_security_walker,
								 context, 0);
	}
	if (IsA(node, BoolExpr))
	{
		BoolExpr *boolean = (BoolExpr *) node;
		ListCell *lc;

		if (boolean->boolop == OR_EXPR)
		{
			foreach(lc, boolean->args)
			{
				if (constant_true((Node *) lfirst(lc)))
				{
					add_rule(context, "constant_tautology", 70);
					break;
				}
			}
		}
	}
	else if (IsA(node, FuncExpr))
	{
		FuncExpr *function = (FuncExpr *) node;

		if (is_named_function(function->funcid, time_functions))
			add_rule(context, "time_function", 80);
		else if (is_named_function(function->funcid, sensitive_functions))
			add_rule(context, "sensitive_function", 70);
	}

	return expression_tree_walker(node, sql_security_walker, context);
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

static void
sql_security_post_parse_analyze(ParseState *pstate, Query *query,
								JumbleState *jstate)
{
	SqlRiskContext context;
	const char *source;

	if (prev_post_parse_analyze_hook)
		prev_post_parse_analyze_hook(pstate, query, jstate);

	if (!sql_injection_enabled || query == NULL)
		return;

	MemSet(&context, 0, sizeof(context));
	initStringInfo(&context.rules);
	source = pstate != NULL ? pstate->p_sourcetext : NULL;

	inspect_query(query, &context);
	(void) query_tree_walker(query, sql_security_walker, &context, 0);

	if (context.has_union)
		add_rule(&context, "union", 25);
	if (context.has_catalog)
		add_rule(&context, "system_catalog", 35);
	if (source != NULL && source_has_stacked_query(source))
		add_rule(&context, "stacked_query", 60);
	if (source != NULL && source_has_comment(source))
		add_rule(&context, "comment", 20);

	if (context.risk >= sql_injection_threshold)
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
	for (entry = strtok_r(copy, ",", &saveptr);
		 entry != NULL;
		 entry = strtok_r(NULL, ",", &saveptr))
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
		if (node->type == XML_ELEMENT_NODE)
		{
			const char *tag = (const char *) node->name;
			xmlAttrPtr attribute;

			if (node != context->wrapper &&
				pg_strcasecmp(tag, "html") != 0 && pg_strcasecmp(tag, "body") != 0)
			{
				if (context->policy == XSS_POLICY_PLAIN_TEXT)
				{
					context->reason = "html markup is not allowed";
					return true;
				}
				if (!token_list_contains(xss_safe_tags, tag))
				{
					context->reason = "HTML tag is not in the safe list";
					return true;
				}
			}

			for (attribute = node->properties;
				 node != context->wrapper && attribute != NULL;
				 attribute = attribute->next)
			{
				const char *name = (const char *) attribute->name;
				xmlChar *value;

				if (pg_strncasecmp(name, "on", 2) == 0 ||
					pg_strcasecmp(name, "style") == 0)
				{
					context->reason = "event or style attribute is not allowed";
					return true;
				}

				if (pg_strcasecmp(name, "href") != 0 &&
					pg_strcasecmp(name, "src") != 0 &&
					pg_strcasecmp(name, "action") != 0 &&
					pg_strcasecmp(name, "formaction") != 0 &&
					pg_strcasecmp(name, "poster") != 0)
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
sql_security_check_write(Relation relation, TupleTableSlot *slot,
						 CmdType operation)
{
	TupleDesc tuple_desc;
	int attribute_number;

	if (prev_ExecutorCheckWrite_hook)
		prev_ExecutorCheckWrite_hook(relation, slot, operation);

	if (!xss_enabled || relation == NULL || slot == NULL)
		return;

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

void
audit_sql_security_init(void)
{
	DefineCustomBoolVariable("audit.sql_injection_detection",
							 "Detect and block SQL injection attack features.",
							 NULL, &sql_injection_enabled, false,
							 PGC_SUSET, 0, NULL, NULL, NULL);
	DefineCustomIntVariable("audit.sql_injection_threshold",
							"Minimum SQL injection risk score to block.",
							NULL, &sql_injection_threshold, 60, 1, 1000,
							PGC_SUSET, 0, NULL, NULL, NULL);
	DefineCustomBoolVariable("audit.xss_detection",
							 "Detect and block XSS content on configured columns.",
							 NULL, &xss_enabled, false,
							 PGC_SUSET, 0, NULL, NULL, NULL);
	DefineCustomStringVariable("audit.xss_columns",
							   "Comma-separated schema.table.column=plain_text|safe_html policies.",
							   NULL, &xss_columns, "", PGC_SUSET, 0,
							   NULL, NULL, NULL);
	DefineCustomStringVariable("audit.xss_safe_tags",
							   "Comma-separated HTML tags allowed by safe_html policies.",
							   NULL, &xss_safe_tags,
							   "p,strong,em,b,i,u,a,ul,ol,li,blockquote,code,pre,br",
							   PGC_SUSET, 0, NULL, NULL, NULL);

	prev_post_parse_analyze_hook = post_parse_analyze_hook;
	post_parse_analyze_hook = sql_security_post_parse_analyze;
	prev_ExecutorCheckWrite_hook = ExecutorCheckWrite_hook;
	ExecutorCheckWrite_hook = sql_security_check_write;
}

void
audit_sql_security_fini(void)
{
	if (post_parse_analyze_hook == sql_security_post_parse_analyze)
		post_parse_analyze_hook = prev_post_parse_analyze_hook;
	if (ExecutorCheckWrite_hook == sql_security_check_write)
		ExecutorCheckWrite_hook = prev_ExecutorCheckWrite_hook;
	if (sql_pending_source != NULL)
	{
		pfree(sql_pending_source);
		sql_pending_source = NULL;
	}
	sql_block_pending = false;
}
