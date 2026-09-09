/*-------------------------------------------------------------------------
 *
 * pl_object_type.c
 *    Oracle object type spec/body parsing support.
 *
 * This module provides the PL/iSQL parser for Oracle-compatible object type
 * specifications and bodies. It handles tokenization of type definitions,
 * extraction of attributes and method declarations, and method body parsing.
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include <ctype.h>

#include "access/htup_details.h"
#include "catalog/pg_namespace.h"
#include "catalog/pg_object_type.h"
#include "catalog/pg_object_type_body.h"
#include "catalog/pg_proc.h"
#include "commands/objecttypecmds.h"
#include "lib/stringinfo.h"
#include "nodes/makefuncs.h"
#include "oracle_parser/ora_scanner.h"
#include "parser/parse_type.h"
#include "parser/scansup.h"
#include "pl_object_type.h"
#include "pl_package.h"
#include "pl_subproc_function.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/objecttypecache.h"
#include "utils/builtins.h"
#include "utils/syscache.h"

/*
 * Token structure for object type scanner.
 * Captures token type, position in source, and text boundaries.
 */
typedef struct ObjectTypeToken
{
	int			token;
	int			location;
	int			length;
	const char *start;
	const char *end;
} ObjectTypeToken;

/*
 * Scanner state for parsing object type definitions.
 * Wraps the Oracle lexer with pushback support for lookahead.
 */
typedef struct ObjectTypeScanner
{
	const char *source;
	ora_core_yyscan_t yyscanner;
	ora_core_yy_extra_type core_yy;
	int			num_pushbacks;
	ObjectTypeToken pushbacks[8];
} ObjectTypeScanner;

static bool is_ident_start(unsigned char ch);
static char *dup_trim_range(const char *start, const char *end);
static PLiSQL_expr *build_method_default_expr(const char *exprsrc);
static PLiSQL_expr *copy_method_default_expr(PLiSQL_expr *expr);
static void object_type_scanner_init(ObjectTypeScanner *scanner,
									 const char *source);
static void object_type_scanner_finish(ObjectTypeScanner *scanner);
static void object_type_scanner_next_token(ObjectTypeScanner *scanner,
										   ObjectTypeToken *token);
static void object_type_scanner_push_back_token(ObjectTypeScanner *scanner,
												const ObjectTypeToken *token);
static bool token_is_char(const ObjectTypeToken *token, int ch);
static bool token_is_identifier(const ObjectTypeToken *token);
static bool token_has_text(const ObjectTypeToken *token, const char *text);
static bool token_is_keyword(const ObjectTypeToken *token, const char *keyword);
static bool token_is_method_modifier(const ObjectTypeToken *token);
static char *token_to_identifier(const ObjectTypeToken *token);
static void validate_constructor_name(const char *type_name,
									  const ObjectTypeMethodDecl *method);
static char *rewrite_constructor_return_statements(const char *source);
static char *normalize_constructor_body_source(const ObjectTypeMethodDecl *method,
											   Oid typnamespace,
											   const char *typname);
static char *normalize_method_body_source(const ObjectTypeMethodDecl *method,
										  Oid typnamespace,
										  const char *typname);
static List *parse_method_parameters(const char *paramsrc);
static FunctionParameter *parse_method_parameter(const char *itemsrc);
static ObjectTypeMethodDecl *parse_method_decl(const char *itemsrc);
static ColumnDef *parse_attribute_decl(const char *itemsrc);
static bool object_type_method_signature_equals(ObjectTypeMethodDecl *left,
											 ObjectTypeMethodDecl *right,
											 Oid typoid,
											 Oid typnamespace,
											 const char *typname);
static Oid get_parent_object_type_oid(Oid objectid);
static void collect_effective_object_type_methods(Oid current_objectid,
											  Oid root_objectid,
											  Oid root_typoid,
											  Oid root_typnamespace,
											  const char *root_typname,
											  List **methods);
static const char *find_body_method_header_end(const char *ptr,
											   const char **after_keyword);
static const char *find_body_method_end(const char *ptr);
static ObjectTypeCacheItem *plisql_compile_object_type(Oid objectid,
													   bool forValidator);
static List *build_method_args(ObjectTypeMethodDecl *method, Oid typoid,
							   Oid typnamespace, const char *typname);
static PLiSQL_type *build_method_rettype(ObjectTypeMethodDecl *method,
										 Oid typoid,
										 Oid typnamespace,
										 const char *typname);
static Oid resolve_method_type(TypeName *typeName, Oid typoid,
							   Oid typnamespace, const char *typname,
							   int32 *typmod);
FuncDetailCode plisql_object_type_lookup(ParseState *pstate,
										 void *object_source,
										 const char *method_name,
										 List **fargs,
										 bool proc_call,
										 List *fargnames,
										 int nargs,
										 Oid *argtypes,
										 Oid *funcid,
										 Oid *rettype,
										 bool *retset,
										 int *nvargs,
										 Oid *vatype,
										 Oid **true_typeids,
										 List **argdefaults,
										 bool has_udt_type);

/*
 * PL/iSQL object type compilation context.
 * Extends the base function structure with object type specific fields
 * for method dispatch and SELF parameter handling.
 */
typedef struct PLiSQL_object_type_source
{
	PLiSQL_function source;
	Oid			objectoid;
	Oid			typoid;
	Oid			bodyoid;
	char	   *type_name;
	PLiSQL_nsitem *special_cur;
} PLiSQL_object_type_source;

/*
 * plisql_parse_object_type_spec
 *    Parse an object type specification source string.
 *    Extracts attributes and method declarations from the type spec.
 *    The spec format is: TYPE name IS OBJECT ( attr1 type1, method1 spec, ... )
 */
ObjectTypeSpec *
plisql_parse_object_type_spec(const char *type_name, const char *objsrc)
{
	ObjectTypeSpec *spec;
	ObjectTypeScanner scanner;
	ObjectTypeToken token;
	const char *item_start;
	int			paren_depth = 0;

	if (objsrc == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("object type \"%s\" has empty specification", type_name)));

	spec = palloc0(sizeof(*spec));
	spec->type_name = pstrdup(type_name);
	spec->is_final = true;

	object_type_scanner_init(&scanner, objsrc);

	PG_TRY();
	{
		/* Expect opening parenthesis, optionally after a captured parent name. */
		object_type_scanner_next_token(&scanner, &token);
		if (!token_is_char(&token, '('))
		{
			for (;;)
			{
				if (token.token == 0)
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("object type \"%s\" specification must contain an attribute list",
									type_name)));

				if (token_is_char(&token, '('))
					break;

				if (!token_is_identifier(&token) && !token_is_char(&token, '.'))
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("unexpected content before object type \"%s\" attribute list",
									type_name)));

				object_type_scanner_next_token(&scanner, &token);
			}
		}

		item_start = token.end;

		/* Parse items within the parentheses */
		for (;;)
		{
			object_type_scanner_next_token(&scanner, &token);

			if (token.token == 0)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("unterminated object type specification for \"%s\"",
								type_name)));

			/* Track nested parentheses for complex type expressions */
			if (token_is_char(&token, '('))
			{
				paren_depth++;
				continue;
			}

			/* End of specification - process final item */
			if (token_is_char(&token, ')'))
			{
				char   *item;

				if (paren_depth > 0)
				{
					paren_depth--;
					continue;
				}

				item = dup_trim_range(item_start, token.start);
				if (item[0] != '\0')
				{
					ObjectTypeMethodDecl *method;

					/* Try to parse as method first, then as attribute */
					method = parse_method_decl(item);
					if (method != NULL)
					{
						validate_constructor_name(type_name, method);
						spec->methods = lappend(spec->methods, method);
					}
					else
						spec->attributes = lappend(spec->attributes,
												 parse_attribute_decl(item));
				}

				object_type_scanner_next_token(&scanner, &token);
				if (token.token != 0)
				{
					if (token_is_keyword(&token, "not"))
					{
						object_type_scanner_next_token(&scanner, &token);
						if (!token_is_keyword(&token, "final"))
							ereport(ERROR,
									(errcode(ERRCODE_SYNTAX_ERROR),
									 errmsg("object type \"%s\" only supports NOT FINAL after the attribute list",
											type_name)));
						spec->is_final = false;
						object_type_scanner_next_token(&scanner, &token);
					}
					else if (token_is_keyword(&token, "final"))
					{
						spec->is_final = true;
						object_type_scanner_next_token(&scanner, &token);
					}

					if (token.token != 0)
						ereport(ERROR,
								(errcode(ERRCODE_SYNTAX_ERROR),
								 errmsg("unexpected trailing content in object type \"%s\" specification",
										type_name)));
				}
				break;
			}

			/* Process item when encountering comma at top level */
			if (token_is_char(&token, ',') && paren_depth == 0)
			{
				char   *item = dup_trim_range(item_start, token.start);

				if (item[0] == '\0')
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("empty object type item in \"%s\" specification",
									type_name)));

				{
					ObjectTypeMethodDecl *method;

					/* Try to parse as method first, then as attribute */
					method = parse_method_decl(item);
					if (method != NULL)
					{
						validate_constructor_name(type_name, method);
						spec->methods = lappend(spec->methods, method);
					}
					else
						spec->attributes = lappend(spec->attributes,
												 parse_attribute_decl(item));
				}

				item_start = token.end;
			}
		}
	}
	PG_CATCH();
	{
		object_type_scanner_finish(&scanner);
		PG_RE_THROW();
	}
	PG_END_TRY();

	object_type_scanner_finish(&scanner);

	return spec;
}

/*
 * plisql_parse_object_type_body
 *    Parse an object type body source string.
 *    Extracts method implementations from the type body.
 *    The body format is: TYPE BODY name IS method1 IS ... END; method2 IS ... END; END;
 */
ObjectTypeBody *
plisql_parse_object_type_body(const char *type_name, const char *bodysrc)
{
	ObjectTypeBody *body;
	const char *ptr;

	if (bodysrc == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("object type body \"%s\" has empty source", type_name)));

	body = palloc0(sizeof(*body));
	body->type_name = pstrdup(type_name);
	ptr = bodysrc;

	/* Parse each method in the body */
	for (;;)
	{
		ObjectTypeScanner scanner;
		const char *header_end;
		const char *body_start;
		const char *method_end;
		char	   *header_src;
		ObjectTypeToken token;
		const char *method_start;

		object_type_scanner_init(&scanner, ptr);

		PG_TRY();
		{
			object_type_scanner_next_token(&scanner, &token);
		}
		PG_CATCH();
		{
			object_type_scanner_finish(&scanner);
			PG_RE_THROW();
		}
		PG_END_TRY();

		object_type_scanner_finish(&scanner);

		/* End of body source reached */
		if (token.token == 0)
			break;

		/* Check for END keyword terminating the body */
		if (token_is_keyword(&token, "end"))
		{
			object_type_scanner_init(&scanner, token.start);

			PG_TRY();
			{
				object_type_scanner_next_token(&scanner, &token);
				object_type_scanner_next_token(&scanner, &token);
				if (token_is_identifier(&token))
					object_type_scanner_next_token(&scanner, &token);

				if (!token_is_char(&token, ';') && token.token != 0)
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("object type body \"%s\" must end with END",
									type_name)));

				object_type_scanner_next_token(&scanner, &token);
				if (token.token != 0)
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("unexpected trailing content in object type body \"%s\"",
									type_name)));
			}
			PG_CATCH();
			{
				object_type_scanner_finish(&scanner);
				PG_RE_THROW();
			}
			PG_END_TRY();

			object_type_scanner_finish(&scanner);
			break;
		}

		/* Parse individual method: extract header and body */
		method_start = token.start;
		header_end = find_body_method_header_end(method_start, &body_start);
		header_src = dup_trim_range(method_start, header_end);
		if (header_src[0] == '\0')
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("invalid empty method header in object type body \"%s\"",
							type_name)));

		method_end = find_body_method_end(body_start);
		{
			ObjectTypeMethodDecl *method = parse_method_decl(header_src);

			validate_constructor_name(type_name, method);
			method->body_src = dup_trim_range(body_start, method_end);
			body->methods = lappend(body->methods, method);
		}
		ptr = method_end;
	}

	return body;
}

/*
 * is_ident_start
 *    Check if a character can start an identifier.
 *    Oracle identifiers can start with letter, underscore, dollar, or hash.
 */
static bool
is_ident_start(unsigned char ch)
{
	return isalpha(ch) || ch == '_' || ch == '$' || ch == '#';
}

/*
 * dup_trim_range
 *    Extract and trim whitespace from a substring.
 *    Returns a palloc'd null-terminated string.
 */
static char *
dup_trim_range(const char *start, const char *end)
{
	while (start < end && isspace((unsigned char) *start))
		start++;
	while (end > start && isspace((unsigned char) end[-1]))
		end--;

	return pnstrdup(start, end - start);
}

/*
 * object_type_scanner_init
 *    Initialize the object type scanner with the given source text.
 *    Sets up the Oracle lexer for tokenizing PL/SQL syntax.
 */
static void
object_type_scanner_init(ObjectTypeScanner *scanner, const char *source)
{
	MemSet(scanner, 0, sizeof(*scanner));
	scanner->source = source;
	scanner->yyscanner = ora_scanner_init_with_ora_keywords(source,
															&scanner->core_yy);
}

/*
 * object_type_scanner_finish
 *    Clean up the scanner and release lexer resources.
 */
static void
object_type_scanner_finish(ObjectTypeScanner *scanner)
{
	if (scanner->yyscanner != NULL)
	{
		ora_scanner_finish(scanner->yyscanner);
		scanner->yyscanner = NULL;
	}
}

/*
 * object_type_scanner_next_token
 *    Get the next token from the scanner.
 *    Handles pushback tokens first, then reads from the lexer.
 */
static void
object_type_scanner_next_token(ObjectTypeScanner *scanner, ObjectTypeToken *token)
{
	ora_core_YYSTYPE lval;

	/* Return pushed-back token if available */
	if (scanner->num_pushbacks > 0)
	{
		scanner->num_pushbacks--;
		*token = scanner->pushbacks[scanner->num_pushbacks];
		return;
	}

	token->token = ora_core_yylex(&lval, &token->location, scanner->yyscanner);
	if (token->token == 0)
	{
		token->length = 0;
		token->start = scanner->source + strlen(scanner->source);
		token->end = token->start;
		return;
	}

	token->length = strlen(scanner->core_yy.scanbuf + token->location);
	token->start = scanner->source + token->location;
	token->end = token->start + token->length;
}

/*
 * object_type_scanner_push_back_token
 *    Push a token back onto the scanner for re-reading.
 *    Used for lookahead in the parser.
 */
static void
object_type_scanner_push_back_token(ObjectTypeScanner *scanner,
									const ObjectTypeToken *token)
{
	if (scanner->num_pushbacks >= lengthof(scanner->pushbacks))
		elog(ERROR, "too many tokens pushed back in object type parser");

	scanner->pushbacks[scanner->num_pushbacks++] = *token;
}

/*
 * Token test functions
 */

/* Check if token is a specific single character */
static bool
token_is_char(const ObjectTypeToken *token, int ch)
{
	return token->token == ch;
}

/* Check if token is an identifier (quoted or unquoted) */
static bool
token_is_identifier(const ObjectTypeToken *token)
{
	return token->length > 0 &&
		(token->start[0] == '"' || is_ident_start((unsigned char) token->start[0]));
}

/* Check if token text matches given string (case-insensitive) */
static bool
token_has_text(const ObjectTypeToken *token, const char *text)
{
	int len = strlen(text);

	if (token->length != len)
		return false;

	return pg_strncasecmp(token->start, text, len) == 0;
}

/* Check if token is a specific keyword (unquoted, case-insensitive) */
static bool
token_is_keyword(const ObjectTypeToken *token, const char *keyword)
{
	return token_is_identifier(token) &&
		token->start[0] != '"' &&
		token_has_text(token, keyword);
}

/* Check if token is a method modifier keyword (DETERMINISTIC, etc.) */
static bool
token_is_method_modifier(const ObjectTypeToken *token)
{
	return token_is_keyword(token, "deterministic") ||
		token_is_keyword(token, "result_cache") ||
		token_is_keyword(token, "parallel_enable") ||
		token_is_keyword(token, "pipelined") ||
		token_is_keyword(token, "accessible") ||
		token_is_keyword(token, "default") ||
		token_is_keyword(token, "invoker");
}

/*
 * token_to_identifier
 *    Convert a token to an identifier string.
 *    Handles quoted identifiers (with escape processing) and unquoted
 *    identifiers (converted to lowercase per Oracle rules).
 */
static char *
token_to_identifier(const ObjectTypeToken *token)
{
	if (!token_is_identifier(token))
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("expected identifier in object type source")));

	/* Process quoted identifier: handle escaped quotes */
	if (token->start[0] == '"')
	{
		StringInfoData buf;
		const char *ptr;

		initStringInfo(&buf);
		for (ptr = token->start + 1; ptr < token->end - 1; ptr++)
		{
			if (*ptr == '"' && ptr + 1 < token->end - 1 && ptr[1] == '"')
			{
				appendStringInfoChar(&buf, '"');
				ptr++;
				continue;
			}
			appendStringInfoChar(&buf, *ptr);
		}
		return buf.data;
	}

	/* Unquoted identifier: convert to lowercase */
	return downcase_truncate_identifier(token->start, token->length, false);
}

/*
 * normalize_constructor_body_source
 *    Normalize constructor body by rewriting RETURN statements.
 *    Converts "RETURN;" to "RETURN SELF;" for proper constructor behavior.
 */
static char *
normalize_constructor_body_source(const ObjectTypeMethodDecl *method,
								  Oid typnamespace,
								  const char *typname)
{
	(void) typnamespace;
	(void) typname;

	return rewrite_constructor_return_statements(method->body_src);
}

/*
 * rewrite_constructor_return_statements
 *    Rewrite bare RETURN statements in constructor bodies to RETURN SELF.
 *    This ensures constructors properly return the initialized object.
 */
static char *
rewrite_constructor_return_statements(const char *source)
{
	ObjectTypeScanner scanner;
	ObjectTypeToken token;
	StringInfoData buf;
	const char *copy_start = source;

	object_type_scanner_init(&scanner, source);
	initStringInfo(&buf);

	PG_TRY();
	{
		for (;;)
		{
			ObjectTypeToken next;

			object_type_scanner_next_token(&scanner, &token);
			if (token.token == 0)
				break;

			if (!token_is_keyword(&token, "return"))
				continue;

			/* Check if RETURN is followed immediately by semicolon */
			object_type_scanner_next_token(&scanner, &next);
			if (!token_is_char(&next, ';'))
			{
				object_type_scanner_push_back_token(&scanner, &next);
				continue;
			}

			/* Replace "RETURN;" with "RETURN SELF;" */
			appendBinaryStringInfo(&buf, copy_start, token.start - copy_start);
			appendStringInfoString(&buf, "return self;");
			copy_start = next.end;
		}
	}
	PG_CATCH();
	{
		object_type_scanner_finish(&scanner);
		pfree(buf.data);
		PG_RE_THROW();
	}
	PG_END_TRY();

	object_type_scanner_finish(&scanner);
	appendStringInfoString(&buf, copy_start);

	return buf.data;
}

/*
 * normalize_method_body_source
 *    Normalize method body source for compilation.
 *    For constructors, rewrites RETURN statements.
 *    For other methods, strips leading MEMBER/STATIC keywords.
 */
static char *
normalize_method_body_source(const ObjectTypeMethodDecl *method,
							 Oid typnamespace,
							 const char *typname)
{
	ObjectTypeScanner scanner;
	ObjectTypeToken token;
	char	   *normalized;

	if (method->body_src == NULL)
		return NULL;

	/* Constructor: rewrite RETURN to RETURN SELF */
	if (method->kind == OBJECT_TYPE_METHOD_CONSTRUCTOR)
		return normalize_constructor_body_source(method, typnamespace, typname);

	/* Strip MEMBER/STATIC prefix if present */
	object_type_scanner_init(&scanner, method->body_src);

	PG_TRY();
	{
		object_type_scanner_next_token(&scanner, &token);
		while (token_is_keyword(&token, "overriding") ||
			   token_is_keyword(&token, "final") ||
			   token_is_keyword(&token, "not"))
		{
			if (token_is_keyword(&token, "not"))
			{
				ObjectTypeToken next;

				object_type_scanner_next_token(&scanner, &next);
				if (!token_is_keyword(&next, "final"))
				{
					object_type_scanner_push_back_token(&scanner, &next);
					break;
				}
			}

			object_type_scanner_next_token(&scanner, &token);
		}

		if (token_is_keyword(&token, "member") ||
			token_is_keyword(&token, "static"))
			normalized = dup_trim_range(token.end,
										method->body_src + strlen(method->body_src));
		else
			normalized = pstrdup(method->body_src);
	}
	PG_CATCH();
	{
		object_type_scanner_finish(&scanner);
		PG_RE_THROW();
	}
	PG_END_TRY();

	object_type_scanner_finish(&scanner);
	return normalized;
}

/*
 * validate_constructor_name
 *    Ensure constructor method name matches the object type name.
 *    Oracle requires constructors to have the same name as their type.
 */
static void
validate_constructor_name(const char *type_name,
						  const ObjectTypeMethodDecl *method)
{
	const char *simple_type_name;

	if (method == NULL ||
		method->kind != OBJECT_TYPE_METHOD_CONSTRUCTOR)
		return;

	/* Extract unqualified type name for comparison */
	simple_type_name = strrchr(type_name, '.');
	if (simple_type_name != NULL)
		simple_type_name++;
	else
		simple_type_name = type_name;

	if (strcmp(method->method_name, simple_type_name) != 0)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("constructor name \"%s\" must match object type \"%s\"",
						method->method_name, type_name)));
}

/*
 * parse_method_parameters
 *    Parse a comma-separated parameter list from method declaration.
 *    Handles nested parentheses for complex type expressions.
 */
static List *
parse_method_parameters(const char *paramsrc)
{
	List	   *parameters = NIL;
	ObjectTypeScanner scanner;
	ObjectTypeToken token;
	const char *item_start = paramsrc;
	int			paren_depth = 0;

	object_type_scanner_init(&scanner, paramsrc);

	PG_TRY();
	{
		for (;;)
		{
			object_type_scanner_next_token(&scanner, &token);

			if (token.token == 0)
			{
				char *item = dup_trim_range(item_start, paramsrc + strlen(paramsrc));

				if (item[0] != '\0')
					parameters = lappend(parameters, parse_method_parameter(item));
				break;
			}

			/* Track nested parentheses */
			if (token_is_char(&token, '('))
			{
				paren_depth++;
				continue;
			}

			if (token_is_char(&token, ')'))
			{
				if (paren_depth > 0)
					paren_depth--;
				continue;
			}

			/* Process parameter at top level comma */
			if (token_is_char(&token, ',') && paren_depth == 0)
			{
				char *item = dup_trim_range(item_start, token.start);

				if (item[0] == '\0')
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("empty method parameter in object type source")));
				parameters = lappend(parameters, parse_method_parameter(item));
				item_start = token.end;
			}
		}
	}
	PG_CATCH();
	{
		object_type_scanner_finish(&scanner);
		PG_RE_THROW();
	}
	PG_END_TRY();

	object_type_scanner_finish(&scanner);

	return parameters;
}

/*
 * parse_method_parameter
 *    Parse a single parameter declaration: name [mode] type [:= default]
 *    Handles IN, OUT, IN OUT modes and NOCOPY modifier.
 */
static FunctionParameter *
parse_method_parameter(const char *itemsrc)
{
	FunctionParameter *parameter;
	ObjectTypeScanner scanner;
	ObjectTypeToken token;
	const char *type_start = itemsrc;
	const char *type_end = itemsrc + strlen(itemsrc);
	const char *default_start = NULL;
	char	   *typestr;
	char	   *defaultstr;
	int			paren_depth = 0;
	bool		saw_default = false;

	parameter = makeNode(FunctionParameter);
	parameter->mode = FUNC_PARAM_IN;
	parameter->defexpr = NULL;

	object_type_scanner_init(&scanner, itemsrc);

	PG_TRY();
	{
		/* Parse parameter name */
		object_type_scanner_next_token(&scanner, &token);
		parameter->name = token_to_identifier(&token);

		/* Parse parameter mode (IN, OUT, IN OUT, INOUT) */
		object_type_scanner_next_token(&scanner, &token);
		if (token_is_keyword(&token, "inout"))
		{
			parameter->mode = FUNC_PARAM_INOUT;
			object_type_scanner_next_token(&scanner, &token);
		}
		else if (token_is_keyword(&token, "in"))
		{
			ObjectTypeToken next;

			object_type_scanner_next_token(&scanner, &next);
			if (token_is_keyword(&next, "out"))
			{
				parameter->mode = FUNC_PARAM_INOUT;
				object_type_scanner_next_token(&scanner, &token);
			}
			else
				token = next;
		}
		else if (token_is_keyword(&token, "out"))
		{
			parameter->mode = FUNC_PARAM_OUT;
			object_type_scanner_next_token(&scanner, &token);
		}

		/* Skip NOCOPY modifier if present */
		if (token_is_keyword(&token, "nocopy"))
			object_type_scanner_next_token(&scanner, &token);

		/* Parse type expression, stopping at default value marker */
		type_start = token.start;
		for (;;)
		{
			if (token.token == 0)
				break;

			if (token_is_char(&token, '('))
				paren_depth++;
			else if (token_is_char(&token, ')'))
			{
				if (paren_depth > 0)
					paren_depth--;
			}
			else if (paren_depth == 0 &&
					 (token_has_text(&token, ":=") ||
					  token_has_text(&token, "=") ||
					  token_is_keyword(&token, "default")))
			{
				saw_default = true;
				type_end = token.start;
				default_start = token.end;
				break;
			}

			object_type_scanner_next_token(&scanner, &token);
		}
	}
	PG_CATCH();
	{
		object_type_scanner_finish(&scanner);
		PG_RE_THROW();
	}
	PG_END_TRY();

	object_type_scanner_finish(&scanner);

	typestr = dup_trim_range(type_start, type_end);

	if (typestr[0] == '\0')
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("parameter \"%s\" is missing a data type", parameter->name)));

	if (saw_default)
	{
		if (parameter->mode != FUNC_PARAM_IN)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_FUNCTION_DEFINITION),
					 errmsg("OUT and IN OUT object type parameters may not have default expressions")));

		defaultstr = dup_trim_range(default_start, itemsrc + strlen(itemsrc));
		if (defaultstr[0] == '\0')
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("parameter \"%s\" has an empty default expression",
							parameter->name)));

		parameter->defexpr = (Node *) build_method_default_expr(defaultstr);
	}

	parameter->argType = typeStringToTypeName(typestr);
	return parameter;
}

static PLiSQL_expr *
build_method_default_expr(const char *exprsrc)
{
	PLiSQL_expr *expr;

	expr = palloc0(sizeof(PLiSQL_expr));
	expr->query = pstrdup(exprsrc);
	expr->parseMode = RAW_PARSE_PLISQL_EXPR;
	expr->plan = NULL;
	expr->paramnos = NULL;
	expr->target_param = -1;
	expr->ns = plisql_ns_top();

	return expr;
}

static PLiSQL_expr *
copy_method_default_expr(PLiSQL_expr *expr)
{
	PLiSQL_expr *newexpr;

	if (expr == NULL)
		return NULL;

	newexpr = palloc0(sizeof(PLiSQL_expr));
	memcpy(newexpr, expr, sizeof(PLiSQL_expr));
	if (expr->query != NULL)
		newexpr->query = pstrdup(expr->query);
	newexpr->plan = NULL;
	newexpr->paramnos = NULL;
	newexpr->expr_simple_expr = NULL;
	newexpr->expr_rw_param = NULL;
	newexpr->expr_simple_plansource = NULL;
	newexpr->expr_simple_plan = NULL;
	newexpr->expr_simple_state = NULL;
	newexpr->expr_simple_in_use = false;
	newexpr->target_param = -1;

	return newexpr;
}

/*
 * parse_method_decl
 *    Parse a method declaration from the object type specification.
 *    Recognizes MEMBER FUNCTION/PROCEDURE, STATIC FUNCTION/PROCEDURE,
 *    and CONSTRUCTOR FUNCTION declarations.
 */
static ObjectTypeMethodDecl *
parse_method_decl(const char *itemsrc)
{
	ObjectTypeMethodDecl *method;
	ObjectTypeScanner scanner;
	ObjectTypeToken token;
	bool		is_method = true;
	bool		is_override = false;
	bool		is_final = false;
	bool		saw_modifier = false;

	object_type_scanner_init(&scanner, itemsrc);

	PG_TRY();
	{
		int			paren_depth = 0;

		object_type_scanner_next_token(&scanner, &token);

		for (;;)
		{
			if (token_is_keyword(&token, "overriding"))
			{
				is_override = true;
				saw_modifier = true;
				object_type_scanner_next_token(&scanner, &token);
				continue;
			}

			if (token_is_keyword(&token, "not"))
			{
				ObjectTypeToken next;

				saw_modifier = true;
				object_type_scanner_next_token(&scanner, &next);
				if (!token_is_keyword(&next, "final"))
					ereport(ERROR,
							(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							 errmsg("unsupported object type method declaration \"%s\"", itemsrc)));
				is_final = false;
				object_type_scanner_next_token(&scanner, &token);
				continue;
			}

			if (token_is_keyword(&token, "final"))
			{
				is_final = true;
				saw_modifier = true;
				object_type_scanner_next_token(&scanner, &token);
				continue;
			}

			break;
		}

		/* Parse MEMBER FUNCTION/PROCEDURE */
		if (token_is_keyword(&token, "member"))
		{
			object_type_scanner_next_token(&scanner, &token);
			if (!token_is_keyword(&token, "function") &&
				!token_is_keyword(&token, "procedure"))
				ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("only MEMBER FUNCTION/PROCEDURE is supported in object type source")));

			method = palloc0(sizeof(*method));
			method->kind = OBJECT_TYPE_METHOD_MEMBER;
			method->is_override = is_override;
			method->is_final = is_final;
		}
		/* Parse STATIC FUNCTION/PROCEDURE */
		else if (token_is_keyword(&token, "static"))
		{
			object_type_scanner_next_token(&scanner, &token);
			if (!token_is_keyword(&token, "function") &&
				!token_is_keyword(&token, "procedure"))
				ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("only STATIC FUNCTION/PROCEDURE is supported in object type source")));

			method = palloc0(sizeof(*method));
			method->kind = OBJECT_TYPE_METHOD_STATIC;
			method->is_override = is_override;
			method->is_final = is_final;
		}
		/* Parse CONSTRUCTOR FUNCTION */
		else if (token_is_keyword(&token, "constructor"))
		{
			object_type_scanner_next_token(&scanner, &token);
			if (!token_is_keyword(&token, "function"))
				ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("only CONSTRUCTOR FUNCTION is supported in object type source")));

			method = palloc0(sizeof(*method));
			method->kind = OBJECT_TYPE_METHOD_CONSTRUCTOR;
			if (saw_modifier)
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_FUNCTION_DEFINITION),
						 errmsg("constructor declarations cannot use OVERRIDING or FINAL")));
		}
		/* Not a method declaration - must be an attribute */
		else
		{
			if (saw_modifier)
				ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("unsupported object type method declaration \"%s\"", itemsrc)));
			if (token_is_keyword(&token, "function") ||
				token_is_keyword(&token, "procedure") ||
				token_is_keyword(&token, "map") ||
				token_is_keyword(&token, "order"))
				ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("unsupported object type method declaration \"%s\"", itemsrc)));

			method = NULL;
			is_method = false;
		}

		/* Parse method name, parameters, and return type */
		if (is_method)
		{
			bool	is_function = token_is_keyword(&token, "function");

			/* Get method name */
			object_type_scanner_next_token(&scanner, &token);
			method->method_name = token_to_identifier(&token);

			/* Parse parameter list if present */
			object_type_scanner_next_token(&scanner, &token);
			if (token_is_char(&token, '('))
			{
				const char *params_start = token.end;
				const char *params_end = token.start;
				char	   *paramsrc;

				for (;;)
				{
					object_type_scanner_next_token(&scanner, &token);
					if (token.token == 0)
						ereport(ERROR,
								(errcode(ERRCODE_SYNTAX_ERROR),
								 errmsg("unterminated parameter list in object type source")));

					if (token_is_char(&token, '('))
					{
						paren_depth++;
						continue;
					}

					if (token_is_char(&token, ')'))
					{
						if (paren_depth == 0)
						{
							params_end = token.start;
							break;
						}
						paren_depth--;
					}
				}

				paramsrc = dup_trim_range(params_start, params_end);
				if (paramsrc[0] != '\0')
					method->parameters = parse_method_parameters(paramsrc);

				object_type_scanner_next_token(&scanner, &token);
			}

			/* Parse return type for functions */
			if (is_function)
			{
				/* Constructor requires RETURN SELF AS RESULT */
				if (method->kind == OBJECT_TYPE_METHOD_CONSTRUCTOR)
				{
					ObjectTypeToken self_token;
					ObjectTypeToken as_token;
					ObjectTypeToken result_token;

					if (!token_is_keyword(&token, "return"))
						ereport(ERROR,
								(errcode(ERRCODE_SYNTAX_ERROR),
								 errmsg("constructor \"%s\" must use RETURN SELF AS RESULT",
										method->method_name)));

					object_type_scanner_next_token(&scanner, &self_token);
					object_type_scanner_next_token(&scanner, &as_token);
					object_type_scanner_next_token(&scanner, &result_token);
					if (!token_is_keyword(&self_token, "self") ||
						!token_is_keyword(&as_token, "as") ||
						!token_is_keyword(&result_token, "result"))
						ereport(ERROR,
								(errcode(ERRCODE_SYNTAX_ERROR),
								 errmsg("constructor \"%s\" must use RETURN SELF AS RESULT",
										method->method_name)));
				}
				/* Regular function: parse return type */
				else
				{
					const char *type_start;
					const char *type_end = itemsrc + strlen(itemsrc);
					int			type_paren_depth = 0;

					if (!token_is_keyword(&token, "return"))
						ereport(ERROR,
								(errcode(ERRCODE_SYNTAX_ERROR),
								 errmsg("method \"%s\" is missing RETURN type",
										method->method_name)));

					object_type_scanner_next_token(&scanner, &token);
					if (token.token == 0)
						ereport(ERROR,
								(errcode(ERRCODE_SYNTAX_ERROR),
								 errmsg("method \"%s\" is missing RETURN type",
										method->method_name)));

					type_start = token.start;

					/* Parse return type, stopping at method modifiers */
					for (;;)
					{
						ObjectTypeToken next = token;

						object_type_scanner_next_token(&scanner, &next);
						if (next.token == 0)
							break;

						if (token_is_char(&next, '('))
						{
							type_paren_depth++;
							continue;
						}

						if (token_is_char(&next, ')'))
						{
							if (type_paren_depth > 0)
								type_paren_depth--;
							continue;
						}

						if (type_paren_depth == 0 && token_is_method_modifier(&next))
						{
							type_end = next.start;
							break;
						}
					}

					{
						char   *typestr = dup_trim_range(type_start, type_end);

						if (typestr[0] == '\0')
							ereport(ERROR,
									(errcode(ERRCODE_SYNTAX_ERROR),
									 errmsg("method \"%s\" is missing RETURN type",
											method->method_name)));

						method->returnType = typeStringToTypeName(typestr);
					}
				}
			}

			method->declsrc = pstrdup(itemsrc);
		}
	}
	PG_CATCH();
	{
		object_type_scanner_finish(&scanner);
		PG_RE_THROW();
	}
	PG_END_TRY();

	object_type_scanner_finish(&scanner);
	return method;
}

/*
 * parse_attribute_decl
 *    Parse an attribute declaration: name type
 *    Returns a ColumnDef node representing the attribute.
 */
static ColumnDef *
parse_attribute_decl(const char *itemsrc)
{
	ColumnDef   *coldef;
	ObjectTypeScanner scanner;
	ObjectTypeToken token;
	const char *type_start;
	char	   *attrname;
	char	   *typestr;

	object_type_scanner_init(&scanner, itemsrc);

	PG_TRY();
	{
		/* Parse attribute name */
		object_type_scanner_next_token(&scanner, &token);
		attrname = token_to_identifier(&token);
		/* Get type start position */
		object_type_scanner_next_token(&scanner, &token);
		if (token.token == 0)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("attribute \"%s\" is missing a data type", attrname)));
		type_start = token.start;
	}
	PG_CATCH();
	{
		object_type_scanner_finish(&scanner);
		PG_RE_THROW();
	}
	PG_END_TRY();

	object_type_scanner_finish(&scanner);

	typestr = pstrdup(type_start);

	coldef = makeNode(ColumnDef);
	coldef->colname = attrname;
	coldef->typeName = typeStringToTypeName(typestr);
	coldef->inhcount = 0;
	coldef->is_local = true;
	coldef->is_not_null = false;
	coldef->is_from_type = false;
	coldef->isvisible = true;
	coldef->not_spefify_column_type = false;

	return coldef;
}

static bool
object_type_method_signature_equals(ObjectTypeMethodDecl *left,
									ObjectTypeMethodDecl *right,
									Oid typoid,
									Oid typnamespace,
									const char *typname)
{
	ListCell   *left_lc;
	ListCell   *right_lc;

	if (strcmp(left->method_name, right->method_name) != 0)
		return false;

	if (left->kind != right->kind)
		return false;

	if (list_length(left->parameters) != list_length(right->parameters))
		return false;

	if ((left->returnType == NULL) != (right->returnType == NULL))
		return false;

	forboth(left_lc, left->parameters, right_lc, right->parameters)
	{
		FunctionParameter *left_param;
		FunctionParameter *right_param;
		int32		left_typmod = -1;
		int32		right_typmod = -1;

		left_param = ((FunctionParameter *) lfirst(left_lc));
		right_param = ((FunctionParameter *) lfirst(right_lc));

		if (left_param->mode != right_param->mode)
			return false;

		if (resolve_method_type(left_param->argType, typoid, typnamespace,
								typname, &left_typmod) !=
			resolve_method_type(right_param->argType, typoid, typnamespace,
								typname, &right_typmod) ||
			left_typmod != right_typmod)
			return false;
	}

	if (left->returnType == NULL)
		return true;

	{
		int32		left_typmod = -1;
		int32		right_typmod = -1;

		return resolve_method_type(left->returnType, typoid, typnamespace,
								   typname, &left_typmod) ==
			resolve_method_type(right->returnType, typoid, typnamespace,
								typname, &right_typmod) &&
			left_typmod == right_typmod;
	}
}

static Oid
get_parent_object_type_oid(Oid objectid)
{
	HeapTuple	objtup;
	Oid			parenttypid;
	Oid			parentobjoid;

	objtup = SearchSysCache1(OBJTYPEOID, ObjectIdGetDatum(objectid));
	if (!HeapTupleIsValid(objtup))
		elog(ERROR, "cache lookup failed for object type %u", objectid);

	parenttypid = ((Form_pg_object_type) GETSTRUCT(objtup))->parenttypid;
	ReleaseSysCache(objtup);

	if (!OidIsValid(parenttypid))
		return InvalidOid;

	parentobjoid = GetSysCacheOid1(OBJTYPETYPOID, Anum_pg_object_type_oid,
								   ObjectIdGetDatum(parenttypid));
	if (!OidIsValid(parentobjoid))
		elog(ERROR, "object type for parent base type %u does not exist", parenttypid);

	return parentobjoid;
}

static void
collect_effective_object_type_methods(Oid current_objectid,
									  Oid root_objectid,
									  Oid root_typoid,
									  Oid root_typnamespace,
									  const char *root_typname,
									  List **methods)
{
	Oid			parentobjoid;
	HeapTuple	objtup;
	HeapTuple	bodytup;
	Form_pg_object_type objform;
	Datum		datum;
	bool		isnull;
	char	   *typname;
	char	   *objsrc;
	char	   *bodysrc;
	ObjectTypeSpec *spec;
	ObjectTypeBody *body;
	ListCell   *lc;

	parentobjoid = get_parent_object_type_oid(current_objectid);
	if (OidIsValid(parentobjoid))
		collect_effective_object_type_methods(parentobjoid,
											 root_objectid,
											 root_typoid,
											 root_typnamespace,
											 root_typname,
											 methods);

	bodytup = SearchSysCache1(OBJTYPEBODYOBJTYPOID,
							  ObjectIdGetDatum(current_objectid));
	if (!HeapTupleIsValid(bodytup))
		return;

	datum = SysCacheGetAttr(OBJTYPEBODYOID, bodytup,
							Anum_pg_object_type_body_bodysrc, &isnull);
	bodysrc = isnull ? NULL : TextDatumGetCString(datum);
	if (bodysrc == NULL)
	{
		ReleaseSysCache(bodytup);
		return;
	}

	objtup = SearchSysCache1(OBJTYPEOID, ObjectIdGetDatum(current_objectid));
	if (!HeapTupleIsValid(objtup))
		elog(ERROR, "cache lookup failed for object type %u", current_objectid);
	objform = (Form_pg_object_type) GETSTRUCT(objtup);
	typname = get_object_type_name(current_objectid);
	datum = SysCacheGetAttr(OBJTYPEOID, objtup,
							Anum_pg_object_type_objsrc, &isnull);
	objsrc = isnull ? NULL : TextDatumGetCString(datum);
	spec = plisql_parse_object_type_spec(typname, objsrc);
	body = plisql_parse_object_type_body(typname, bodysrc);

	foreach(lc, body->methods)
	{
		ObjectTypeMethodDecl *method;
		ListCell   *spec_lc;
		ListCell   *existing_lc;

		method = ((ObjectTypeMethodDecl *) lfirst(lc));
		if (current_objectid != root_objectid &&
			method->kind == OBJECT_TYPE_METHOD_CONSTRUCTOR)
			continue;

		/*
		 * Defaults belong to the public declaration and need not be repeated
		 * in the body.  Match against this type's spec before merging inherited
		 * methods, so overloads and constructors get their own defaults.
		 */
		foreach(spec_lc, spec->methods)
		{
			ObjectTypeMethodDecl *spec_method = lfirst(spec_lc);
			ListCell   *spec_param_lc;
			ListCell   *body_param_lc;

			if (!object_type_method_signature_equals(spec_method, method,
													objform->typid,
													objform->objnamespace, typname))
				continue;

			forboth(spec_param_lc, spec_method->parameters,
					body_param_lc, method->parameters)
			{
				FunctionParameter *spec_param = lfirst(spec_param_lc);
				FunctionParameter *body_param = lfirst(body_param_lc);

				if (spec_param->defexpr != NULL)
					body_param->defexpr = spec_param->defexpr;
			}
			break;
		}

		existing_lc = NULL;
		foreach(existing_lc, *methods)
		{
			ObjectTypeMethodDecl *existing_method;

			existing_method = ((ObjectTypeMethodDecl *) lfirst(existing_lc));
			if (object_type_method_signature_equals(existing_method,
													method,
													root_typoid,
													root_typnamespace,
													root_typname))
			{
				lfirst(existing_lc) = method;
				break;
			}
		}

		if (existing_lc == NULL)
			*methods = lappend(*methods, method);
	}

	ReleaseSysCache(objtup);
	ReleaseSysCache(bodytup);
}

/*
 * find_body_method_header_end
 *    Find the end of a method header (IS or AS keyword) in type body.
 *    Handles special case of "AS RESULT" in constructor declarations.
 */
static const char *
find_body_method_header_end(const char *ptr, const char **after_keyword)
{
	ObjectTypeScanner scanner;
	ObjectTypeToken token;
	const char *result = NULL;
	int			paren_depth = 0;

	object_type_scanner_init(&scanner, ptr);

	PG_TRY();
	{
		for (;;)
		{
			object_type_scanner_next_token(&scanner, &token);

			if (token.token == 0)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("unterminated method definition in object type body")));

			/* Track parentheses for parameter list handling */
			if (token_is_char(&token, '('))
				paren_depth++;
			else if (token_is_char(&token, ')'))
			{
				if (paren_depth > 0)
					paren_depth--;
			}
			/* Look for IS or AS keyword at top level */
			else if (paren_depth == 0 &&
					 (token_is_keyword(&token, "is") ||
					  token_is_keyword(&token, "as")))
			{
				/* Skip "AS" in "RETURN SELF AS RESULT" */
				if (token_is_keyword(&token, "as"))
				{
					ObjectTypeToken lookahead;

					object_type_scanner_next_token(&scanner, &lookahead);
					if (token_is_keyword(&lookahead, "result"))
						continue;

					object_type_scanner_push_back_token(&scanner, &lookahead);
				}

				*after_keyword = token.end;
				result = token.start;
				break;
			}
		}
	}
	PG_CATCH();
	{
		object_type_scanner_finish(&scanner);
		PG_RE_THROW();
	}
	PG_END_TRY();

	object_type_scanner_finish(&scanner);
	return result;
}

/*
 * find_body_method_end
 *    Find the end of a method body in the type body source.
 *    Tracks nested blocks (BEGIN/END, FUNCTION/PROCEDURE, CASE) to find
 *    the matching END for the current method.
 */
static const char *
find_body_method_end(const char *ptr)
{
	ObjectTypeScanner scanner;
	ObjectTypeToken token;
	const char *result = NULL;
	int			blocklevel = 1;
	int			pending_subprogram_headers = 1;

	object_type_scanner_init(&scanner, ptr);

	PG_TRY();
	{
		for (;;)
		{
			object_type_scanner_next_token(&scanner, &token);
			if (token.token == 0)
				break;

			/* BEGIN keyword: decrement pending headers or increment block level */
			if (token_is_keyword(&token, "begin"))
			{
				if (pending_subprogram_headers > 0)
					pending_subprogram_headers--;
				else
					blocklevel++;
				continue;
			}

			/* Nested subprogram increases block level */
			if (token_is_keyword(&token, "function") ||
				token_is_keyword(&token, "procedure"))
			{
				blocklevel++;
				pending_subprogram_headers++;
				continue;
			}

			/* CASE increases block level */
			if (token_is_keyword(&token, "case"))
			{
				blocklevel++;
				continue;
			}

			/* END keyword: handle various END forms */
			if (token_is_keyword(&token, "end"))
			{
				ObjectTypeToken lookahead;

				object_type_scanner_next_token(&scanner, &lookahead);

				/* END IF and END LOOP don't close blocks */
				if (token_is_keyword(&lookahead, "if") ||
					token_is_keyword(&lookahead, "loop"))
				{
					ObjectTypeToken semi;

					object_type_scanner_next_token(&scanner, &semi);
					if (!token_is_char(&semi, ';'))
						ereport(ERROR,
								(errcode(ERRCODE_SYNTAX_ERROR),
								 errmsg("unterminated method body in object type body")));
					continue;
				}

				/* END; closes current block */
				if (token_is_char(&lookahead, ';'))
				{
					blocklevel--;
					if (blocklevel == 0)
					{
						result = lookahead.end;
						break;
					}
					continue;
				}

				/* END CASE label; or END label; closes block */
				if (token_is_keyword(&lookahead, "case") ||
					token_is_identifier(&lookahead))
				{
					ObjectTypeToken next;

					object_type_scanner_next_token(&scanner, &next);
					if (token_is_char(&next, ';'))
					{
						blocklevel--;
						if (blocklevel == 0)
						{
							result = next.end;
							break;
						}
						continue;
					}

					blocklevel--;
					if (blocklevel == 0)
						ereport(ERROR,
								(errcode(ERRCODE_SYNTAX_ERROR),
								 errmsg("unterminated method body in object type body")));
					object_type_scanner_push_back_token(&scanner, &next);
					object_type_scanner_push_back_token(&scanner, &lookahead);
					continue;
				}

				blocklevel--;
				if (blocklevel == 0)
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("unterminated method body in object type body")));
				object_type_scanner_push_back_token(&scanner, &lookahead);
			}
		}
	}
	PG_CATCH();
	{
		object_type_scanner_finish(&scanner);
		PG_RE_THROW();
	}
	PG_END_TRY();

	object_type_scanner_finish(&scanner);

	if (result != NULL)
		return result;

	ereport(ERROR,
			(errcode(ERRCODE_SYNTAX_ERROR),
			 errmsg("unterminated method body in object type body")));
	return NULL;
}

/*
 * plisql_object_type_handle
 *    Get or compile an object type given its OID.
 *    If is_body is true, first resolves the body OID to its parent type OID.
 *    Uses the object type cache to avoid recompilation.
 */
void *
plisql_object_type_handle(Oid objectid, bool is_body)
{
	if (is_body)
	{
		HeapTuple	bodytup;

		bodytup = SearchSysCache1(OBJTYPEBODYOID, ObjectIdGetDatum(objectid));
		if (HeapTupleIsValid(bodytup))
		{
			objectid = ((Form_pg_object_type_body) GETSTRUCT(bodytup))->objtypoid;
			ReleaseSysCache(bodytup);
		}
	}

	/* Return cached version if valid, otherwise compile */
	if (ObjectTypeCacheIsValid(&objectid, true))
		return ObjectTypeCacheLookup(&objectid);

	return plisql_compile_object_type(objectid, false);
}

void
plisql_object_type_validator(Oid objectid, bool is_body)
{
	int					rc;
	Oid					objoid = objectid;
	ObjectTypeCacheItem *item;
	PLiSQL_object_type_source *source;
	int					i;

	if ((rc = SPI_connect()) != SPI_OK_CONNECT)
		elog(ERROR, "SPI_connect failed: %s", SPI_result_code_string(rc));

	if (is_body)
	{
		HeapTuple	bodytup;

		bodytup = SearchSysCache1(OBJTYPEBODYOID, ObjectIdGetDatum(objectid));
		if (!HeapTupleIsValid(bodytup))
			elog(ERROR, "cache lookup failed for object type body %u", objectid);
		objoid = ((Form_pg_object_type_body) GETSTRUCT(bodytup))->objtypoid;
		ReleaseSysCache(bodytup);
	}

	item = plisql_compile_object_type(objoid, true);
	if (item != NULL)
	{
		source = (PLiSQL_object_type_source *) item->source;
		for (i = 0; i < source->source.nsubprocfuncs; i++)
		{
			PLiSQL_subproc_function *subprocfunc = source->source.subprocfuncs[i];

			if (subprocfunc->function->action == NULL)
				(void) plisql_dynamic_compile_subproc(NULL, subprocfunc, true);
		}

		MemoryContextDelete(item->ctxt);
		pfree(item);
	}

	if ((rc = SPI_finish()) != SPI_OK_FINISH)
		elog(ERROR, "SPI_finish failed: %s", SPI_result_code_string(rc));
}

/*
 * plisql_get_object_method_func
 *    Retrieve the PL/iSQL function for an object type method call.
 *    Handles cache lookup and dynamic compilation of methods on first use.
 */
PLiSQL_function *
plisql_get_object_method_func(FunctionCallInfo fcinfo, bool forValidator)
{
	FuncExpr			   *funcexpr;
	ObjectTypeCacheItem	   *item;
	PLiSQL_object_type_source *source;
	PLiSQL_subproc_function *subprocfunc;
	PLiSQL_function		   *func;
	int						fno;

	funcexpr = (FuncExpr *) fcinfo->flinfo->fn_expr;
	Assert(funcexpr->function_from == FUNC_FROM_OBJECT);

	if (!OidIsValid(funcexpr->pkgoid))
		elog(ERROR, "object method funcexpr has invalid object oid");

	/* Get object type from cache or compile it */
	if (ObjectTypeCacheIsValid(&funcexpr->pkgoid, true))
		item = ObjectTypeCacheLookup(&funcexpr->pkgoid);
	else
		item = ObjectTypeHandle(funcexpr->pkgoid, false);

	if (item == NULL || item->source == NULL)
		elog(ERROR, "object type %u cache is not found", funcexpr->pkgoid);

	source = (PLiSQL_object_type_source *) item->source;
	funcexpr->parent_func = item->source;

	/* Get method by slot number (stored in funcid during planning) */
	fno = (int) funcexpr->funcid;
	if (fno < 0 || fno >= source->source.nsubprocfuncs)
		elog(ERROR, "invalid object method slot %d", fno);

	subprocfunc = source->source.subprocfuncs[fno];
	/* Compile method body on first call if not already compiled */
	if (subprocfunc->function->action == NULL)
		func = plisql_dynamic_compile_subproc(fcinfo, subprocfunc, forValidator);
	else
		func = subprocfunc->function;

	source->source.use_count++;
	return func;
}

/*
 * release_object_method_usecount
 *    Decrement the use count for an object type method after execution.
 *    Called when method execution completes to track resource usage.
 */
void
release_object_method_usecount(FunctionCallInfo fcinfo)
{
	FuncExpr			   *funcexpr;
	ObjectTypeCacheItem	   *item;
	PLiSQL_object_type_source *source;

	funcexpr = (FuncExpr *) fcinfo->flinfo->fn_expr;
	Assert(funcexpr->function_from == FUNC_FROM_OBJECT);

	if (!OidIsValid(funcexpr->pkgoid))
		return;

	item = ObjectTypeCacheLookup(&funcexpr->pkgoid);
	if (item == NULL || item->source == NULL)
		return;

	source = (PLiSQL_object_type_source *) item->source;
	if (source->source.use_count > 0)
		source->source.use_count--;
}

/*
 * plisql_compile_object_type
 *    Compile an object type body into executable form.
 *    Parses the body source, builds subprogram functions for each method,
 *    and stores the result in the object type cache.
 */
static ObjectTypeCacheItem *
plisql_compile_object_type(Oid objectid, bool forValidator)
{
	ObjectTypeCacheItem	   *item;
	PLiSQL_object_type_source *source;
	HeapTuple				objtup;
	HeapTuple				bodytup;
	Form_pg_object_type		objform;
	Form_pg_object_type_body bodyform;
	char				   *typname;
	MemoryContext			oldctx;
	PLiSQL_variable		   *var;
	List				   *effective_methods = NIL;
	ListCell			   *lc;

	/* Look up object type and body from system catalogs */
	objtup = SearchSysCache1(OBJTYPEOID, ObjectIdGetDatum(objectid));
	if (!HeapTupleIsValid(objtup))
		elog(ERROR, "cache lookup failed for object type %u", objectid);

	bodytup = SearchSysCache1(OBJTYPEBODYOBJTYPOID, ObjectIdGetDatum(objectid));
	if (!HeapTupleIsValid(bodytup))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
				 errmsg("object type \"%s\" has no body",
						get_object_type_name(objectid))));

	objform = (Form_pg_object_type) GETSTRUCT(objtup);
	bodyform = (Form_pg_object_type_body) GETSTRUCT(bodytup);
	typname = pstrdup(get_typename(objform->typid));

	/* Create cache item with its own memory context */
	item = MemoryContextAllocZero(TopMemoryContext, sizeof(*item));
	item->ctxt = AllocSetContextCreate(TopMemoryContext,
									   "PL/iSQL object type",
									   ALLOCSET_DEFAULT_SIZES);
	source = MemoryContextAllocZero(item->ctxt, sizeof(*source));

	/* Initialize cache item metadata */
	item->key = objectid;
	item->object_xmin = HeapTupleHeaderGetRawXmin(objtup->t_data);
	item->object_tid = objtup->t_self;
	item->body_xmin = HeapTupleHeaderGetRawXmin(bodytup->t_data);
	item->body_tid = bodytup->t_self;
	item->object_hash_value =
		GetSysCacheHashValue1(OBJTYPEOID, ObjectIdGetDatum(objectid));
	item->body_hash_value =
		GetSysCacheHashValue1(OBJTYPEBODYOID, ObjectIdGetDatum(bodyform->oid));
	item->cachestatus = OBJECT_TYPE_SET_BODY_FLAG(0);
	item->source = source;

	/* Initialize source function structure */
	source->objectoid = objectid;
	source->typoid = objform->typid;
	source->bodyoid = bodyform->oid;
	source->type_name = MemoryContextStrdup(item->ctxt, typname);
	source->source.fn_signature =
		MemoryContextStrdup(item->ctxt, quote_qualified_identifier(NULL, typname));
	source->source.fn_oid = objectid;
	source->source.fn_is_trigger = PLISQL_NOT_TRIGGER;
	source->source.fn_input_collation = InvalidOid;
	source->source.fn_cxt = item->ctxt;
	source->source.out_param_varno = -1;
	source->source.resolve_option = plisql_variable_conflict;
	source->source.print_strict_params = plisql_print_strict_params;
	source->source.extra_warnings = 0;
	source->source.extra_errors = 0;
	source->source.fn_prokind = PROKIND_FUNCTION;
	source->source.fn_readonly = false;
	source->source.sql_varno = -1;
	source->source.found_varno = -1;

	/* Set up compilation context and switch to item's memory context */
	plisql_push_compile_global_proper();
	oldctx = MemoryContextSwitchTo(item->ctxt);

	PG_TRY();
	{
		/* Initialize namespace and compilation state */
		plisql_ns_init();
		plisql_ns_push(typname, PLISQL_LABEL_BLOCK);
		plisql_DumpExecTree = false;
		plisql_check_syntax = forValidator;
		cur_compile_func_level = 0;
		plisql_saved_compile[cur_compile_func_level] = &source->source;
		plisql_curr_compile = &source->source;
		plisql_compile_tmp_cxt = item->ctxt;
		plisql_error_funcname = MemoryContextStrdup(item->ctxt, typname);
		plisql_IdentifierLookup = IDENTIFIER_LOOKUP_DECLARE;
		plisql_start_datums();
		plisql_start_subproc_func();
		collect_effective_object_type_methods(objectid,
											  objectid,
											  objform->typid,
											  objform->objnamespace,
											  typname,
											  &effective_methods);

		/* Build FOUND variable */
		var = plisql_build_variable("found", 0,
									plisql_build_datatype(BOOLOID, -1,
														 InvalidOid, NULL),
									true);
		source->source.found_varno = var->dno;

		/* Build SQL cursor variable */
		var = plisql_build_variable("sql", 0,
									plisql_build_datatype(SYS_REFCURSOROID, -1,
														 InvalidOid, NULL),
									true);
		source->source.sql_varno = var->dno;

		/* Build subprogram function for each method */
		foreach(lc, effective_methods)
		{
			ObjectTypeMethodDecl	  *method;
			PLiSQL_subproc_function	  *subprocfunc;

			method = ((ObjectTypeMethodDecl *) lfirst(lc));
			subprocfunc = plisql_build_subproc_function(method->method_name,
														build_method_args(method,
																		 source->typoid,
																		 objform->objnamespace,
																		 typname),
														build_method_rettype(method,
																		 source->typoid,
																		 objform->objnamespace,
																		 typname),
														-1);
			subprocfunc->function->fn_is_object_constructor =
				(method->kind == OBJECT_TYPE_METHOD_CONSTRUCTOR);
			subprocfunc->lastoutsubprocfno = plisql_nsubprocFuncs;
			subprocfunc->lastoutvardno = plisql_nDatums;
			subprocfunc->global_cur = plisql_ns_top();
			subprocfunc->src = MemoryContextStrdup(item->ctxt,
												  normalize_method_body_source(method,
																			   objform->objnamespace,
																			   typname));
		}

		/* Finalize datums and subprograms */
		plisql_finish_datums(&source->source);
		plisql_finish_subproc_func(&source->source);

		/* Propagate shared state to all subprogram functions */
		for (int i = 0; i < source->source.nsubprocfuncs; i++)
		{
			PLiSQL_subproc_function *subprocfunc = source->source.subprocfuncs[i];
			PLiSQL_function *parentfunc = subprocfunc->function;

			parentfunc->ndatums = source->source.ndatums;
			parentfunc->datums = source->source.datums;
			parentfunc->nsubprocfuncs = source->source.nsubprocfuncs;
			parentfunc->subprocfuncs = source->source.subprocfuncs;
			parentfunc->found_varno = source->source.found_varno;
			parentfunc->sql_varno = source->source.sql_varno;
			parentfunc->fn_cxt = source->source.fn_cxt;
			parentfunc->fn_input_collation = source->source.fn_input_collation;
			parentfunc->fn_is_trigger = source->source.fn_is_trigger;
			parentfunc->fn_prokind = source->source.fn_prokind;
			parentfunc->fn_oid = source->source.fn_oid;
		}
		source->special_cur = plisql_ns_top();
	}
	PG_CATCH();
	{
		MemoryContextSwitchTo(oldctx);
		plisql_pop_compile_global_proper();
		ReleaseSysCache(bodytup);
		ReleaseSysCache(objtup);
		PG_RE_THROW();
	}
	PG_END_TRY();

	MemoryContextSwitchTo(oldctx);
	plisql_pop_compile_global_proper();

	/* Insert into cache, replacing any existing entry */
	if (!forValidator)
	{
		if (ObjectTypeCacheLookup(&objectid) != NULL)
			ObjectTypeCacheDelete(&objectid);
		ObjectTypeCacheInsert(&objectid, item);
	}

	ReleaseSysCache(bodytup);
	ReleaseSysCache(objtup);
	return item;
}

/*
 * build_method_args
 *    Build the argument list for a method from its declaration.
 *    Adds implicit SELF parameter for MEMBER and CONSTRUCTOR methods.
 */
static List *
build_method_args(ObjectTypeMethodDecl *method, Oid typoid,
				  Oid typnamespace, const char *typname)
{
	List	   *args = NIL;
	ListCell   *lc;

	/* Add SELF parameter for member and constructor methods */
	if (method->kind == OBJECT_TYPE_METHOD_MEMBER ||
		method->kind == OBJECT_TYPE_METHOD_CONSTRUCTOR)
	{
		PLiSQL_function_argitem *selfarg = palloc0(sizeof(*selfarg));

		selfarg->argname = pstrdup("self");
		selfarg->type = plisql_build_datatype(typoid, -1, InvalidOid, NULL);
		/*
		 * Oracle member procedures implicitly receive SELF as a writable
		 * IN OUT parameter so attribute updates can flow back to the caller.
		 * Member functions keep SELF read-only, while constructors still use
		 * the existing constructor-specific assignment handling.
		 */
		if (method->kind == OBJECT_TYPE_METHOD_MEMBER &&
			method->returnType == NULL)
			selfarg->argmode = ARGMODE_INOUT;
		else
			selfarg->argmode = ARGMODE_IN;
		args = lappend(args, selfarg);
	}

	/* Process declared parameters */
	foreach(lc, method->parameters)
	{
		FunctionParameter		 *param;
		PLiSQL_function_argitem *argitem;
		int32					 typmod = -1;

		param = ((FunctionParameter *) lfirst(lc));
		argitem = palloc0(sizeof(*argitem));
		argitem->argname = pstrdup(param->name);
		argitem->type = plisql_build_datatype(resolve_method_type(param->argType,
																 typoid,
																 typnamespace,
																 typname,
																 &typmod),
											  typmod,
											  InvalidOid,
											  param->argType);
		switch (param->mode)
		{
			case FUNC_PARAM_OUT:
				argitem->argmode = ARGMODE_OUT;
				break;
			case FUNC_PARAM_INOUT:
				argitem->argmode = ARGMODE_INOUT;
				break;
			default:
				argitem->argmode = ARGMODE_IN;
				break;
		}
		if (param->defexpr != NULL)
			argitem->defexpr = copy_method_default_expr((PLiSQL_expr *) param->defexpr);
		args = lappend(args, argitem);
	}

	return args;
}

/*
 * build_method_rettype
 *    Build the return type for a method.
 *    Constructors return the object type; others use declared return type.
 */
static PLiSQL_type *
build_method_rettype(ObjectTypeMethodDecl *method, Oid typoid,
					 Oid typnamespace, const char *typname)
{
	int32		typmod = -1;
	Oid			typeid;

	/* Constructors return the object type itself */
	if (method->kind == OBJECT_TYPE_METHOD_CONSTRUCTOR)
		return plisql_build_datatype(typoid, -1, InvalidOid, NULL);

	if (method->returnType == NULL)
		return NULL;

	typeid = resolve_method_type(method->returnType, typoid, typnamespace,
								 typname, &typmod);
	return plisql_build_datatype(typeid, typmod, InvalidOid,
								 method->returnType);
}

/*
 * resolve_method_type
 *    Resolve a type name in method context, handling SELF type references.
 *    If the type name matches the object type name, returns the object type OID.
 */
static Oid
resolve_method_type(TypeName *typeName, Oid typoid,
					Oid typnamespace, const char *typname, int32 *typmod)
{
	char   *rawtype;

	*typmod = -1;

	/* Check if the type name refers to the object type itself (SELF reference) */
	if (typeName != NULL &&
		typeName->names != NIL &&
		typeName->typeOid == InvalidOid &&
		typeName->pct_type == false &&
		typeName->setof == false &&
		typeName->arrayBounds == NIL)
	{
		char   *resolved_schema = NULL;
		char   *resolved_type = NULL;

		DeconstructQualifiedName(typeName->names, &resolved_schema, &resolved_type);
		if (resolved_type != NULL &&
			strcmp(resolved_type, typname) == 0 &&
			(resolved_schema == NULL ||
			 strcmp(resolved_schema, get_namespace_name(typnamespace)) == 0))
			return typoid;
	}

	/* Otherwise, parse as a regular type string */
	rawtype = TypeNameToString(typeName);
	parseTypeString(rawtype, &typoid, typmod, true);
	return typoid;
}

/*
 * plisql_object_type_lookup
 *    Look up a method in an object type by name.
 *    Searches the namespace for the method and returns function details.
 */
FuncDetailCode
plisql_object_type_lookup(ParseState *pstate,
						  void *object_source,
						  const char *method_name,
						  List **fargs,
						  bool proc_call,
						  List *fargnames,
						  int nargs,
						  Oid *argtypes,
						  Oid *funcid,
						  Oid *rettype,
						  bool *retset,
						  int *nvargs,
						  Oid *vatype,
						  Oid **true_typeids,
						  List **argdefaults,
						  bool has_udt_type)
{
	PLiSQL_object_type_source *source;
	PLiSQL_nsitem		   *nse;

	source = (PLiSQL_object_type_source *) object_source;

	/* Look up method in the object type namespace */
	nse = plisql_ns_lookup(source->special_cur, false,
						   method_name, NULL, NULL, NULL);
	if (nse == NULL ||
		(nse->itemtype != PLISQL_NSTYPE_SUBPROC_FUNC &&
		 nse->itemtype != PLISQL_NSTYPE_SUBPROC_PROC))
		return FUNCDETAIL_NOTFOUND;

	/* Get function details from the subprogram */
	return plisql_get_subprocfunc_detail(pstate,
										 &source->source,
										 nse,
										 (char *) method_name,
										 fargs,
										 proc_call,
										 fargnames,
										 nargs,
										 argtypes,
										 funcid,
										 rettype,
										 retset,
										 nvargs,
										 vatype,
										 true_typeids,
										 argdefaults,
										 has_udt_type,
										 false);
}
