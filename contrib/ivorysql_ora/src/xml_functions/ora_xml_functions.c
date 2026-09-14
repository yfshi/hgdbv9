/*-------------------------------------------------------------------------------
 *
 * File: ora_xml_functions.c
 *
 * Abstract:
 * 		This file implements the C functions that support the XML SQL functions
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *		contrib/ivorysql_ora/src/xml_functions/ora_xml_functions.c
 *
 *-------------------------------------------------------------------------------
 */
#include "postgres.h"

#include <ctype.h>
#include <math.h>
#include "fmgr.h"
#include "access/htup_details.h"
#include "catalog/pg_type.h"
#include "funcapi.h"
#include "libpq/pqformat.h"
#include "nodes/nodeFuncs.h"
#include "nodes/supportnodes.h"
#include "optimizer/optimizer.h"
#include "port/pg_bitutils.h"
#include "utils/array.h"
#include "utils/arrayaccess.h"
#include "utils/builtins.h"
#include "utils/datum.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/selfuncs.h"
#include "utils/typcache.h"
#include "access/table.h"
#include "catalog/namespace.h"
#include "catalog/pg_class.h"
#include "commands/dbcommands.h"
#include "executor/spi.h"
#include "executor/tablefunc.h"
#include "lib/stringinfo.h"
#include "libpq/pqformat.h"
#include "mb/pg_wchar.h"
#include "miscadmin.h"
#include "nodes/execnodes.h"
#include "utils/date.h"
#include "utils/datetime.h"
#include "utils/rel.h"
#include "utils/syscache.h"
#include "utils/arrayaccess.h"
#include "utils/xml.h"

#include "../include/ivorysql_ora.h"

#ifdef USE_LIBXML
#include <libxml/chvalid.h>
#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/tree.h>
#include <libxml/uri.h>
#include <libxml/xmlerror.h>
#include <libxml/xmlversion.h>
#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#endif

/*
 * Local definitions
 */
#define NO_XML_SUPPORT() \
	ereport(ERROR, \
			(errcode(ERRCODE_FEATURE_NOT_SUPPORTED), \
			 errmsg("unsupported XML feature"), \
			 errdetail("This functionality requires the server to be built with libxml support."), \
			 errhint("You need to rebuild PostgreSQL using %s.", "--with-libxml")))

PgXmlErrorContext *ivy_xml_parser_init(PgXmlStrictness strictness);

#ifdef USE_LIBXML
typedef struct xpath_ws
{
	xmlDocPtr			doc;
	xmlXPathContextPtr 	xpathctx;
	xmlXPathCompExprPtr xpathcomp;
	xmlXPathObjectPtr 	xpathobj;
	/* Begin - BUG#Z9218 */
	xmlDocPtr			tempdoc;	/* temporary docoment owning new nodes */
	MemoryContextCallback callback;	/* free libxml resources on context reset/delete */
	/* End - BUG#Z9218 */
} xpath_ws;
#endif

/*
 * ----------------------------------------------------------------------------
 *	Helper Function declarations
 * ----------------------------------------------------------------------------
 */
#ifdef USE_LIBXML
static text *rv_newline(text *tv);
static char *left_trim(char *str);
static char *right_trim(char *str);
static char *ivy_trim(char *str);
static char *ivy_escape_xml(const char *str);
static xmltype *ivy_stringinfo_to_xmltype(StringInfo buf);
static void cleanup_ws(xpath_ws *ws);
static xmlXPathObjectPtr ivy_xml_xpath(xpath_ws *ws, text *xpath_expr_text, xmltype *data, char *namespace);
static xmlXPathObjectPtr ivy_xml_xpath2(xpath_ws *ws, text *xpath_expr_text, xmlDocPtr doc, char *namespace);
static void init_ws_doc(xpath_ws *ws, xmltype *data);
static xmlDocPtr ivy_xml_parse_content(const char *data, int len, int options);
static void register_ns_from_csting(xmlXPathContextPtr xpathCtx, char* nsList);
static int ivy_xpathobjnode_nr(xmlXPathObjectPtr xpathobj);
static char *ivy_xmlnode_getcontent(xmlNodePtr cur);
static char *ivy_xml_xpathobjtostring(xmlXPathObjectPtr xpathobj);
static text *ivy_xml_xmlnode2xmltype(xmlNodePtr cur);
static xmltype *ivy_xml_xpathobjtoxmltype(xmlXPathObjectPtr xpathobj);
static void ivy_xml_setnodecontent(xmlXPathObjectPtr xpathobj, xmlChar *var);
static void ivy_xml_delenode(xmlXPathObjectPtr xpathobj);
static void ivy_xml_addchildnode(xmlXPathObjectPtr xpathobj, xmlNodePtr cnode, bool allow_document);
static void ivy_xml_addprevsiblingnode(xmlXPathObjectPtr xpathobj, const xmlChar* new_node);
static void ivy_xml_addnextsiblingnode(xmlXPathObjectPtr xpathobj, const xmlChar* new_node);
static bool ivy_xml_replacenode(xpath_ws *ws, const xmlChar* new_node);
static xmlChar *ivy_xmlCharStrndup(const char *str, size_t len);
#endif

#ifdef USE_LIBXML
/*
 * Initialize for xml parsing.
 *
 * As with the underlying pg_xml_init function, calls to this MUST be followed
 * by a PG_TRY block that guarantees that pg_xml_done is called.
 */
PgXmlErrorContext *
ivy_xml_parser_init(PgXmlStrictness strictness)
{
	PgXmlErrorContext *xmlerrcxt;

	/* Set up error handling (we share the core's error handler) */
	xmlerrcxt = pg_xml_init(strictness);

	/* Note: we're assuming an elog cannot be thrown by the following calls */

	/* Initialize libxml */
	xmlInitParser();

	xmlSubstituteEntitiesDefault(1);
	xmlLoadExtDtdDefaultValue = 1;

	return xmlerrcxt;
}

/* Begin - BUG#Z9218 */
static void
ivy_xml_free_tempdoc(xpath_ws *ws)
{
	if (ws->tempdoc)
		xmlFreeDoc(ws->tempdoc);
	ws->tempdoc = NULL;
}
/* End - BUG#Z9218 */

/*
 * cleanup_ws
 *	clean up after processing the result of using xpath
 */
static void
cleanup_ws(xpath_ws *ws)
{
	if (ws->xpathobj)
		xmlXPathFreeObject(ws->xpathobj);
	ws->xpathobj = NULL;
	if (ws->xpathcomp)
		xmlXPathFreeCompExpr(ws->xpathcomp);
	ws->xpathcomp = NULL;
	if (ws->xpathctx)
		xmlXPathFreeContext(ws->xpathctx);
	ws->xpathctx = NULL;
	if (ws->doc)
		xmlFreeDoc(ws->doc);
	ws->doc = NULL;
	ivy_xml_free_tempdoc(ws);
}

/* Begin - BUG#Z9218 */
static void
ivy_xml_workspace_reset(void *arg)
{
	cleanup_ws((xpath_ws *) arg);
}

/*
 * libxml allocations are not reclaimed by PostgreSQL's error rollback.
 * Keep cleanup armed even when errors occur after successful XPath evaluation.
 * Both the callback and its argument must outlive the C function's stack.
 */
static xpath_ws *
ivy_xml_workspace(void)
{
	xpath_ws *ws = palloc0(sizeof(xpath_ws));

	ws->callback.func = ivy_xml_workspace_reset;
	ws->callback.arg = ws;
	MemoryContextRegisterResetCallback(CurrentMemoryContext, &ws->callback);
	return ws;
}

/*
 * libxml can return a tree after a namespace error; that is not valid XMLType.
 */
static xmlDocPtr
ivy_xml_parse_document(const char *data, int len, int options)
{
	xmlParserCtxtPtr ctxt;
	xmlDocPtr doc;

	ctxt = xmlNewParserCtxt();
	if (ctxt == NULL)
		ereport(ERROR, (errcode(ERRCODE_OUT_OF_MEMORY),
				errmsg("could not allocate XML parser context")));
	doc = xmlCtxtReadMemory(ctxt, data, len, NULL, NULL,
						options | XML_PARSE_NONET | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
	if (doc != NULL && (!ctxt->wellFormed || !ctxt->nsWellFormed))
	{
		xmlFreeDoc(doc);
		doc = NULL;
	}
	xmlFreeParserCtxt(ctxt);
	return doc;
}

/*
 * XPath names are UTF-8; non-ASCII bytes cannot delimit an ASCII axis name.
 */
static bool
ivy_xml_xpath_namechar(unsigned char c)
{
	return c >= 128 || (c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
		c == '_' || c == '-' || c == '.';
}

static bool
ivy_xml_xpath_space(char c)
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

/*
 * Oracle's namespace axis omits the predefined xml binding.  Apply the
 * filter to the axis step, BEFORE user predicates or outer positional
 * filters.  Scanning tokens also keeps string literals untouched.
 */
static xmlChar *
ivy_xml_oracle_xpath(text *expression)
{
	char *input = text_to_cstring(expression);
	const char *p = input;
	StringInfoData buf;
	bool blank = true;

	/*
	 * Reject whitespace-only expressions, let libxml handle an empty string.
	 */
	for (; *p; p++)
		if (!ivy_xml_xpath_space(*p))
			blank = false;
	if (blank && p != input)
		ereport(ERROR, (errcode(ERRCODE_INVALID_XML_DOCUMENT),
				errmsg("XML parsing failed")));

	initStringInfo(&buf);

	/*
	 * Scan Xpath tokens for namespace-axis steps: namespace::*, namespace::p
	 * (a namespace prefix), or namespace::node(), allowing XPath whitespace
	 * around the axis separator and inside node(). Insert the xml-binding
	 * filter before any user predicate, e.g: namespace::*[1] becomes
	 * namepace::*[name() != 'xml'][1], so positions count only visible bindings.
	 *
	 * 	SELECT extract(XMLTYPE('<a xmlns:x="urn:x"/>'), '//namespace::*') from dual;
	 *
	 *	rewrite the Xpath to:
	 *		//namespace::*[name() != 'xml']
	 *	based on the current expected result, the result is urn:x, similarly
	 *	//namespace::xml is rewrite as:
	 *		//namespace::xml[name() != 'xml']
	 *
	 * Exclude xml before location filtering, while recognizing different naming conventions.
	 *
	 *		Xpath							Rewrite						Expected
	 *------------------------------------------------------------------------------
	 * (//namespace::*)[1]		(//namespace::*[name() != 'xml'])[1]	 urn:x
	 * (//namespace::*)[2]		(//namespace::*[name() != 'xml'])[1]	 null
	 * //namespace::*[1]		//namespace::*[name() != 'xml'][1]		 urn:x
	 * //namespace::node()		//namespace::node()[name() != 'xml']	 urn:x
	 */
	for (p = input; *p;)
	{
		const char *start = p;

		/*
		 * Copy quoted literals verbatim, even if they contain namespace::*.
		 */
		if (*p == '\'' || *p == '"')
		{
			char quote = *p++;

			while (*p && *p != quote)
				p++;
			if (*p)
				p++;
		}
		else if (ivy_xml_xpath_namechar((unsigned char) *p))
		{
			/*
			 * Match a whole axis name, not part of a longer or qualified name.
			 */
			while (ivy_xml_xpath_namechar((unsigned char) *p))
				p++;
			if (p - start == 9 && strncmp(start, "namespace", 9) == 0 &&
				(start == input || start[-1] != ':'))
			{
				const char *axis = p;
				const char *test;

				while (ivy_xml_xpath_space(*axis))
					axis++;
				if (axis[0] == ':' && axis[1] == ':')
				{
					axis += 2;
					while (ivy_xml_xpath_space(*axis))
						axis++;
					test = axis;
					if (*axis == '*')
						axis++;
					else
						while (ivy_xml_xpath_namechar((unsigned char) *axis))
							axis++;
					if (axis - test == 4 && strncmp(test, "node", 4) == 0)
					{
						const char *end = axis;

						while (ivy_xml_xpath_space(*end))
							end++;
						if (*end == '(')
						{
							end++;
							while (ivy_xml_xpath_space(*end))
								end++;
							if (*end == ')')
								axis = end + 1;
						}
					}
					/* Leave invalid node tests for libxml's syntax checker. */
					{
						const char *end = axis;

						while (ivy_xml_xpath_space(*end))
							end++;
						if (*end == '(')
							axis = test;
					}
					if (axis > test)
					{
						appendBinaryStringInfo(&buf, start, axis - start);
						appendStringInfoString(&buf, "[name() != 'xml']");
						p = axis;
						continue;
					}
				}
			}
		}
		else
			p++;
		appendBinaryStringInfo(&buf, start, p - start);
	}
	pfree(input);
	return (xmlChar *) buf.data;
}

/*
 * XMLType can contain text or several top-level elements.  Try a document
 * first to retain declarations/DTDs.  For content, use an element context:
 * libxml2 2.9's document context silently drops top-level character data.
 * The temporary element is never visible to XPath or to serialization.
 * Callers pass a NUL-terminated buffer as well as its byte length.
 */
static xmlDocPtr
ivy_xml_parse_content(const char *data, int len, int options)
{
	xmlDocPtr	doc;
	xmlNodePtr	context;
	xmlNodePtr	nodes = NULL;
	int			rc;

	options |= XML_PARSE_NONET | XML_PARSE_NOERROR | XML_PARSE_NOWARNING;
	doc = ivy_xml_parse_document(data, len, options);
	if (doc != NULL)
		return doc;

	/* Serialized XMLType content can carry an XML declaration too. */
	if (len > 5 && strncmp(data, "<?xml", 5) == 0 &&
		(data[5] == ' ' || data[5] == '\t' || data[5] == '\r' || data[5] == '\n'))
	{
		const char *end = strstr(data, "?>");
		StringInfoData header;
		int decl_len;

		if (end == NULL)
			return NULL;
		decl_len = end + 2 - data;
		initStringInfo(&header);
		appendBinaryStringInfo(&header, data, decl_len);
		appendStringInfoString(&header, "<xmltype_context/>");
		/* Let libxml validate the declaration instead of parsing it ourselves. */
		doc = ivy_xml_parse_document(header.data, header.len, options);
		pfree(header.data);
		if (doc == NULL)
			return NULL;
		context = xmlDocGetRootElement(doc);
		data += decl_len;
		len -= decl_len;
	}
	else
	{
		doc = xmlNewDoc(BAD_CAST "1.0");
		if (doc == NULL)
			return NULL;
		context = xmlNewDocNode(doc, NULL, BAD_CAST "xmltype_context", NULL);
		if (context == NULL)
		{
			xmlFreeDoc(doc);
			return NULL;
		}
		xmlDocSetRootElement(doc, context);
	}

	/*
	 * xmlParseInNodeContext does not expose nsWellFormed (even on success).
	 * Validate the fragment in the same empty element scope first.
	 */
	{
		StringInfoData wrapped;
		xmlDocPtr checked;

		initStringInfo(&wrapped);
		appendStringInfoString(&wrapped, "<xmltype_context>");
		appendBinaryStringInfo(&wrapped, data, len);
		appendStringInfoString(&wrapped, "</xmltype_context>");
		checked = ivy_xml_parse_document(wrapped.data, wrapped.len, options);
		pfree(wrapped.data);
		if (checked == NULL)
		{
			xmlFreeDoc(doc);
			return NULL;
		}
		xmlFreeDoc(checked);
	}
	rc = xmlParseInNodeContext(context, data, len, options, &nodes);
	xmlUnlinkNode(context);
	xmlFreeNode(context);
	if (rc != XML_ERR_OK)
	{
		xmlFreeNodeList(nodes);
		xmlFreeDoc(doc);
		return NULL;
	}
	if (nodes != NULL && xmlAddChildList((xmlNodePtr) doc, nodes) == NULL)
	{
		xmlFreeNodeList(nodes);
		xmlFreeDoc(doc);
		return NULL;
	}
	return doc;
}
/* End - BUG#Z9218 */

/*
 * ivy_xml_xpath
 *	build the xpath and return the xpath object
 */
static xmlXPathObjectPtr
ivy_xml_xpath(xpath_ws *ws, text *xpath_expr_text, xmltype *data, char *namespace)
{
	PgXmlErrorContext *xmlerrcxt;
	char	   *datastr;
	int32		len;
	int32		xpath_len;
	xmlChar    *string;
	xmlChar    *xpath_expr;

	ws->doc = NULL;
	ws->xpathcomp = NULL;
	ws->xpathctx = NULL;
	ws->xpathobj = NULL;

	datastr = VARDATA(data);
	len = VARSIZE(data) - VARHDRSZ;
	xpath_len = VARSIZE_ANY_EXHDR(xpath_expr_text);
	if (xpath_len == 0)
		ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
				 errmsg("empty XPath expression")));

	string = ivy_xmlCharStrndup(datastr, len);
	xpath_expr = ivy_xml_oracle_xpath(xpath_expr_text);

	xmlerrcxt = ivy_xml_parser_init(PG_XML_STRICTNESS_LEGACY);

	PG_TRY();
	{
		ws->doc = ivy_xml_parse_content((const char *)string, len, XML_PARSE_NOBLANKS); /* Bug#Z202 */
		if (ws->doc != NULL)
		{
			ws->xpathctx = xmlXPathNewContext(ws->doc);
			if (ws->xpathctx == NULL)
				ereport(ERROR,(errcode(ERRCODE_OUT_OF_MEMORY),
						errmsg("could not allocate XPath context")));
			ws->xpathctx->node = (xmlNodePtr)ws->doc;

			/* register namespaces, if any */
			if (namespace)
				register_ns_from_csting(ws->xpathctx, namespace);

			ws->xpathcomp = xmlXPathCompile(xpath_expr);
			if (ws->xpathcomp == NULL)
				ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
						errmsg("invalid XPath expression")));

			ws->xpathobj = xmlXPathCompiledEval(ws->xpathcomp, ws->xpathctx);
			if (ws->xpathobj == NULL)
				ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
						errmsg("could not create XPath object")));
		}
		else
			ereport(ERROR,(errcode(ERRCODE_INVALID_XML_DOCUMENT),
					errmsg("could not parse XML document")));
	}
	PG_CATCH();
	{
		cleanup_ws(ws);
		pg_xml_done(xmlerrcxt, true);
		PG_RE_THROW();
	}
	PG_END_TRY();

	if (ws->xpathobj == NULL)
		cleanup_ws(ws);

	pg_xml_done(xmlerrcxt, false);

	return ws->xpathobj;
}

static void
init_ws_doc(xpath_ws *ws, xmltype *data)
{
	PgXmlErrorContext *xmlerrcxt;
	char	   *datastr;
	xmlChar    *string;
	int32		len;

	ws->doc = NULL;
	ws->xpathcomp = NULL;
	ws->xpathctx = NULL;
	ws->xpathobj = NULL;

	datastr = VARDATA(data);
	len = VARSIZE(data) - VARHDRSZ;
	string = ivy_xmlCharStrndup(datastr, len);

	xmlerrcxt = ivy_xml_parser_init(PG_XML_STRICTNESS_LEGACY);

	PG_TRY();
	{
		ws->doc = ivy_xml_parse_content((const char *)string, len, XML_PARSE_NOBLANKS); /* Bug#Z202 */
		if (ws->doc == NULL)
			ereport(ERROR,(errcode(ERRCODE_INVALID_XML_DOCUMENT),
					errmsg("could not parse XML document")));
	}
	PG_CATCH();
	{
		cleanup_ws(ws);
		pg_xml_done(xmlerrcxt, true);
		PG_RE_THROW();
	}
	PG_END_TRY();

	pg_xml_done(xmlerrcxt, false);
}

static xmlXPathObjectPtr
ivy_xml_xpath2(xpath_ws *ws, text *xpath_expr_text, xmlDocPtr doc, char *namespace)
{
	PgXmlErrorContext *xmlerrcxt;
	int32		xpath_len;
	xmlChar    *xpath_expr = NULL;

	xpath_len = VARSIZE_ANY_EXHDR(xpath_expr_text);
	if (xpath_len == 0)
		ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
				 errmsg("empty XPath expression")));
	xpath_expr = ivy_xml_oracle_xpath(xpath_expr_text);

	/* updatexml can evaluate several XPath expressions against one document. */
	if (ws->xpathobj)
		xmlXPathFreeObject(ws->xpathobj);
	ws->xpathobj = NULL;
	if (ws->xpathcomp)
		xmlXPathFreeCompExpr(ws->xpathcomp);
	ws->xpathcomp = NULL;
	if (ws->xpathctx)
		xmlXPathFreeContext(ws->xpathctx);
	ws->xpathctx = NULL;

	xmlerrcxt = ivy_xml_parser_init(PG_XML_STRICTNESS_LEGACY);

	PG_TRY();
	{
		ws->xpathctx = xmlXPathNewContext(doc);
		if (ws->xpathctx == NULL)
			ereport(ERROR,(errcode(ERRCODE_OUT_OF_MEMORY),
					errmsg("could not allocate XPath context")));
		ws->xpathctx->node = (xmlNodePtr)doc;

		/* register namespaces, if any */
		if (namespace)
			register_ns_from_csting(ws->xpathctx, namespace);

		ws->xpathcomp = xmlXPathCompile(xpath_expr);
		if (ws->xpathcomp == NULL)
			ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
					errmsg("invalid XPath expression")));

		ws->xpathobj = xmlXPathCompiledEval(ws->xpathcomp, ws->xpathctx);
		if (ws->xpathobj == NULL)
			ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
					errmsg("could not create XPath object")));
	}
	PG_CATCH();
	{
		cleanup_ws(ws);
		pg_xml_done(xmlerrcxt, true);
		PG_RE_THROW();
	}
	PG_END_TRY();
	pg_xml_done(xmlerrcxt, false);

	return ws->xpathobj;
}
#endif

/*
 * ----------------------------------------------------------------------------
 *				Exported Function declarations
 * ----------------------------------------------------------------------------
 */
PG_FUNCTION_INFO_V1(ivy_xmltype_in);
PG_FUNCTION_INFO_V1(ivy_xmltype_out);

PG_FUNCTION_INFO_V1(ivy_existsnode);
PG_FUNCTION_INFO_V1(ivy_existsnode2);
PG_FUNCTION_INFO_V1(ivy_extractvalue);
PG_FUNCTION_INFO_V1(ivy_extractvalue2);
PG_FUNCTION_INFO_V1(ivy_extract);
PG_FUNCTION_INFO_V1(ivy_extract2);
PG_FUNCTION_INFO_V1(ivy_deletexml);
PG_FUNCTION_INFO_V1(ivy_deletexml2);
PG_FUNCTION_INFO_V1(ivy_appendchildxml);
PG_FUNCTION_INFO_V1(ivy_appendchildxml2);
PG_FUNCTION_INFO_V1(ivy_insertxmlbefore);
PG_FUNCTION_INFO_V1(ivy_insertxmlbefore2);
PG_FUNCTION_INFO_V1(ivy_insertxmlafter);
PG_FUNCTION_INFO_V1(ivy_insertxmlafter2);
PG_FUNCTION_INFO_V1(ivy_insertchildxml);
PG_FUNCTION_INFO_V1(ivy_insertchildxml2);
PG_FUNCTION_INFO_V1(ivy_insertchildxmlbefore);
PG_FUNCTION_INFO_V1(ivy_insertchildxmlbefore2);
PG_FUNCTION_INFO_V1(ivy_insertchildxmlafter);
PG_FUNCTION_INFO_V1(ivy_insertchildxmlafter2);
/*
 * ----------------------------------------------------------------------------
 *	Helper Function definition
 * ----------------------------------------------------------------------------
 */
#ifdef USE_LIBXML
/*
 * rv_newline
 *	Remove the final newline in XMLTYPE data.
 */
static text*
rv_newline(text *tv)
{
	char 	*tmp = NULL;
	int		 len = 0;
	text	*ret = NULL;

	if (!tv)
		return ret;

	tmp = text_to_cstring(tv);
	len = strlen(tmp);
	if (len > 0 && tmp[len-1] == '\n')
		tmp[len-1] = '\0';
	ret = cstring_to_text(tmp);

	return ret;
}

/*
 * left_trim
 * 	remove the spaces at the beginning of a string
 */
static char *
left_trim(char *str)
{
	char	*start = str;
	char	*tmp = str;

	while (isspace((unsigned char) *start)) start++;
	while ((*tmp++ = *start++));

	return str;
}

/*
 * right_trim
 *	remove the spaces at the end of a string
 */
static char *
right_trim(char *str)
{
	size_t	len = strlen(str);

	while (len > 0 && isspace((unsigned char) str[len - 1]))
		len--;
	str[len] = '\0';

	return str;
}

/*
 * ivy_trim
 * 	trim spaces at the beginning and end of a sring
 */
static char *
ivy_trim(char *str)
{
	str = left_trim(str);
	str = right_trim(str);

	return str;
}

/*
 * Escape characters in text that have special meanings in XML.
 *
 * Returns a palloc'd string.
 *
 * NB: this is intentionally not dependent on libxml.
 */
static char *
ivy_escape_xml(const char *str)
{
	StringInfoData buf;
	const char *p;

	initStringInfo(&buf);
	for (p = str; *p; p++)
	{
		switch (*p)
		{
		case '&':
			appendStringInfoString(&buf, "&amp;");
			break;
		case '<':
			appendStringInfoString(&buf, "&lt;");
			break;
		case '>':
			appendStringInfoString(&buf, "&gt;");
			break;
		case '\r':
			appendStringInfoString(&buf, "&#x0d;");
			break;
		default:
			appendStringInfoCharMacro(&buf, *p);
			break;
		}
	}
	return buf.data;
}

static xmltype *
ivy_stringinfo_to_xmltype(StringInfo buf)
{
	return (xmltype *) cstring_to_text_with_len(buf->data, buf->len);
}

static void
register_ns_from_csting(xmlXPathContextPtr xpathCtx, char* nsList)
{
	char	*p = nsList;

	while (*p != '\0')
	{
		char   *prefix_start;
		char   *url_start;
		char   *prefix;
		char   *url;

		while (isspace((unsigned char) *p))
			p++;
		if (*p == '\0')
			break;

		if (strncmp(p, "xmlns:", 6) != 0)
			elog(ERROR, "Invalid namespace");
		p += 6;
		prefix_start = p;
		while (*p != '\0' && *p != '=' &&
			   !isspace((unsigned char) *p))
			p++;
		if (p == prefix_start || *p != '=' || p[1] != '"')
			elog(ERROR, "Invalid namespace");

		prefix = pnstrdup(prefix_start, p - prefix_start);
		p += 2;
		url_start = p;
		while (*p != '\0' && *p != '"')
			p++;
		if (*p != '"')
			elog(ERROR, "Invalid namespace");
		url = pnstrdup(url_start, p - url_start);
		p++;

		if (*p != '\0' && !isspace((unsigned char) *p))
			elog(ERROR, "Invalid namespace");

		if (xmlXPathRegisterNs(xpathCtx, (xmlChar *) prefix,
							   (xmlChar *) url) != 0)
			elog(ERROR, "unable to register NS with prefix=%s and url=%s",
				 prefix, url);

		pfree(prefix);
		pfree(url);
	}
}

static int
ivy_xpathobjnode_nr(xmlXPathObjectPtr xpathobj)
{
	int			result = 0;

	if (xpathobj->type == XPATH_NODESET)
	{
		if (xpathobj->nodesetval != NULL &&
			xpathobj->nodesetval->nodeTab != NULL)
			result = xpathobj->nodesetval->nodeNr;
	}

	return result;
}

static char *
ivy_xmlnode_getcontent(xmlNodePtr cur)
{
	char    	*result = NULL;

	if (cur != NULL && cur->type == XML_ELEMENT_NODE)
	{
		if (cur->children && cur->children->type == XML_TEXT_NODE)
			result = (char *)xmlNodeGetContent(cur->children);
	}

	return result;
}

static char *
ivy_xml_xpathobjtostring(xmlXPathObjectPtr xpathobj)
{
	int			num = 0;
	char	   *res = NULL;
	char	   *res_str = NULL;
	int			i;
	StringInfoData buf;

	if (xpathobj->type == XPATH_NODESET)
	{
		if (xpathobj->nodesetval != NULL &&
			xpathobj->nodesetval->nodeTab != NULL)
		{
			num = xpathobj->nodesetval->nodeNr;
			initStringInfo(&buf);

			for (i = 0; i < num; i++)
			{
				res = ivy_xmlnode_getcontent(xpathobj->nodesetval->nodeTab[i]);
				if (res)
				{
					appendStringInfoString(&buf, res);
					xmlFree(res);
				}
				else
					appendStringInfoString(&buf, "");
			}
			res_str = buf.data;
		}
	}

	return res_str;
}

static text *
ivy_xml_xmlnode2xmltype(xmlNodePtr cur)
{
	xmltype    *result = NULL;

	if (cur == NULL)
		ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
				errmsg("XML node is null")));
	if (cur->type == XML_NAMESPACE_DECL)
	{
		/* XPath namespace results are xmlNs, not ordinary xmlNode structs. */
		xmlNsPtr ns = (xmlNsPtr) cur;
		char *escaped = ivy_escape_xml((const char *) ns->href);
		text *value = cstring_to_text(escaped);

		pfree(escaped);
		return value;
	}

	if (cur->type != XML_ATTRIBUTE_NODE && cur->type != XML_TEXT_NODE)
	{
		void		(*volatile nodefree) (xmlNodePtr) = NULL;
		volatile xmlBufferPtr buf = NULL;
		volatile xmlNodePtr cur_copy = NULL;

		PG_TRY();
		{
			int			bytes;
			int			format = 1;

			if (cur->type == XML_DOCUMENT_NODE)
			{
				xmlNodePtr child;

				for (child = cur->children; child != NULL; child = child->next)
					if (child->type == XML_TEXT_NODE || child->type == XML_CDATA_SECTION_NODE)
						format = 0;
			}
			buf = xmlBufferCreate();
			if (buf == NULL)
				ereport(ERROR, (errcode(ERRCODE_OUT_OF_MEMORY),
						errmsg("could not allocate xmlBuffer")));

			cur_copy = xmlCopyNode(cur, 1);
			if (cur_copy == NULL)
				ereport(ERROR, (errcode(ERRCODE_OUT_OF_MEMORY),
						errmsg("could not copy node")));

			nodefree = (cur_copy->type == XML_DOCUMENT_NODE) ?
				(void (*) (xmlNodePtr)) xmlFreeDoc : xmlFreeNode;

			if (format == 0)
			{
				xmlNodePtr child;

				/* Dumping a document inserts newlines even with format=0. */
				bytes = 0;
				for (child = cur_copy->children; child != NULL; child = child->next)
				{
					bytes = xmlNodeDump(buf, (xmlDocPtr) cur_copy, child, 0, 0);
					if (bytes == -1)
						break;
				}
				/* Match the document serializer's final newline convention. */
				if (bytes != -1 && xmlBufferAdd(buf, BAD_CAST "\n", 1) != 0)
					bytes = -1;
			}
			else
				bytes = xmlNodeDump(buf, NULL, cur_copy, 0, 1); /* Bug#Z202 */
			if (bytes == -1)
				ereport(ERROR, (errcode(ERRCODE_OUT_OF_MEMORY),
						errmsg("could not dump node")));

			result = (xmltype *)cstring_to_text_with_len((const char *) xmlBufferContent(buf),
														 xmlBufferLength(buf));
		}
		PG_FINALLY();
		{
			if (nodefree)
				nodefree(cur_copy);
			if (buf)
				xmlBufferFree(buf);
		}
		PG_END_TRY();
	}
	else
	{
		xmlChar    *str;

		str = xmlXPathCastNodeToString(cur);
		if (str == NULL)
			ereport(ERROR, (errcode(ERRCODE_OUT_OF_MEMORY),
					errmsg("could not convert XML node to string")));
		PG_TRY();
		{
			/* Here we rely on XML having the same representation as TEXT */
			char	*escaped = ivy_escape_xml((char *) str);
			result = (xmltype *) cstring_to_text(escaped);
			pfree(escaped);
		}
		PG_FINALLY();
		{
			xmlFree(str);
		}
		PG_END_TRY();
	}

	return result;
}

static xmltype *
ivy_xml_xpathobjtoxmltype(xmlXPathObjectPtr xpathobj)
{
	xmlNodePtr	cur = NULL;
	char	   *str = NULL;
	int			num = 0;
	int			i;
	StringInfoData buf;

	initStringInfo(&buf);

	if (xpathobj->type == XPATH_NODESET)
	{
		if (xpathobj->nodesetval != NULL &&
			xpathobj->nodesetval->nodeTab != NULL)
		{
			num = xpathobj->nodesetval->nodeNr;
			for (i = 0; i < num; i++)
			{
				cur = xpathobj->nodesetval->nodeTab[i];
				if (cur == NULL)
					continue;
				if (cur->type == XML_TEXT_NODE)
				{
					str = (char *)xmlNodeGetContent(cur);
					if (str == NULL)
						ereport(ERROR, (errcode(ERRCODE_OUT_OF_MEMORY),
								errmsg("could not get XML node content")));
					appendStringInfoString(&buf, str);
					xmlFree(str);
				}
				else
				{
					str = text_to_cstring(ivy_xml_xmlnode2xmltype(cur));
					appendStringInfoString(&buf, str);
					if (cur->type != XML_DOCUMENT_NODE)
						appendStringInfoString(&buf, "\n");
					pfree(str);
				}
			}
		}
	}

	return ivy_stringinfo_to_xmltype(&buf);
}

static void
ivy_xml_setnodecontent(xmlXPathObjectPtr xpathobj, xmlChar *var)
{
	xmlNodePtr	cur = NULL;
	int			num = 0;
	int			i;

	if (xpathobj->type == XPATH_NODESET)
	{
		if (xpathobj->nodesetval != NULL &&
			xpathobj->nodesetval->nodeTab != NULL)
		{
			num = xpathobj->nodesetval->nodeNr;
			/* Clear descendants before their ancestors release child lists. */
			for (i = num - 1; i >= 0; i--)
			{
				cur = xpathobj->nodesetval->nodeTab[i];
				if (cur == NULL)
					continue;
				/* Namespace copies refer back to their owner.  Release them
				 * while that owner is still alive, before clearing ancestors. */
				if (cur->type == XML_NAMESPACE_DECL)
				{
					xmlXPathNodeSetFreeNs((xmlNsPtr) cur);
					xpathobj->nodesetval->nodeTab[i] = NULL;
					continue;
				}
				xpathobj->nodesetval->nodeTab[i] = NULL;
				if (strcmp((char *)var, "") == 0 && cur->type == XML_ELEMENT_NODE)
				{
					if (cur->children != NULL)
					{
						xmlNodePtr child = cur->children;

						cur->children = NULL;
						cur->last = NULL;
						xmlFreeNodeList(child);
					}
				}
				else
				{
					if (cur->type == XML_TEXT_NODE)
						xmlNodeSetContent(cur, var);
				}
			}
		}
	}

}

static void
ivy_xml_delenode(xmlXPathObjectPtr xpathobj)
{
	int			num = 0;
	int			i;

	if (xpathobj->type == XPATH_NODESET &&
		xpathobj->nodesetval != NULL &&
		xpathobj->nodesetval->nodeTab != NULL)
	{
		num = xpathobj->nodesetval->nodeNr;
		/*
		 * XPath node sets are in document order.  Delete in reverse order so
		 * that deleting an ancestor cannot leave later entries dangling.
		 */
		for (i = num - 1; i >= 0; i--)
		{
			xmlNodePtr cur = xpathobj->nodesetval->nodeTab[i];

			if (cur != NULL)
			{
				if (cur->type == XML_DOCUMENT_NODE)
					ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
								errmsg("XML node '' (type=document) does not support this operation")));
				if (cur->type == XML_NAMESPACE_DECL)
					ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
							errmsg("invalid XPath expression")));
				/* Do not leave freed nodes for xmlXPathFreeNodeSet to inspect. */
				xpathobj->nodesetval->nodeTab[i] = NULL;
				if (cur->type == XML_ATTRIBUTE_NODE)
					xmlRemoveProp((xmlAttrPtr) cur);
				else
				{
					xmlUnlinkNode(cur);
					xmlFreeNode(cur);
				}
			}
		}
	}
}

/*
 * Oracle's legacy child-before/after functions compose the paths literally:
 * '/' + '/' + 'b' becomes '//b', while the document element's parent
 * followed by '/b' stays a root-child lookup.  Trailing whitespace after a
 * slash at this boundary is rejected as XML parsing failed by Oracle 21c,
 * even though libxml accepts it.
 */
static void
ivy_xml_check_child_path(const char *parent)
{
	int			len = strlen(parent);
	int			end = len;

	while(end > 0 && ivy_xml_xpath_space(parent[end-1]))
		end--;
	if (end > 0 && end < len && parent[end - 1] == '/')
		ereport(ERROR, (errcode(ERRCODE_INVALID_XML_DOCUMENT),
						errmsg("XML parsing failed")));
}

static void
ivy_xml_addchildnode(xmlXPathObjectPtr xpathobj, xmlNodePtr cnode, bool allow_document)
{
	int			num = 0;
	int			i;
	xmlNodePtr	source = NULL;

	if (cnode != NULL && cnode->type == XML_DOCUMENT_NODE)
		source = xmlDocGetRootElement((xmlDocPtr) cnode);
	else
		source = cnode;

	if (source == NULL)
		ereport(ERROR, (errcode(ERRCODE_INVALID_XML_DOCUMENT),
				errmsg("XML new node has no root element")));

	if (xpathobj->type == XPATH_NODESET)
	{
		if (xpathobj->nodesetval != NULL &&
			xpathobj->nodesetval->nodeTab != NULL)
		{
			num = xpathobj->nodesetval->nodeNr;
			for (i = 0; i < num; i++)
			{
				xmlNodePtr target = xpathobj->nodesetval->nodeTab[i];
				xmlNodePtr copy;

				if (target == NULL)
					continue;
				if (target->type != XML_ELEMENT_NODE &&
					!(allow_document && target->type == XML_DOCUMENT_NODE))
					ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
							errmsg("invalid XPath expression")));
				copy = xmlDocCopyNode(source,
									  target->type == XML_DOCUMENT_NODE ?
									  (xmlDocPtr) target : target->doc, 1);
				if (copy == NULL)
					ereport(ERROR, (errcode(ERRCODE_OUT_OF_MEMORY),
							errmsg("could not copy XML new node")));
				if (xmlAddChild(target, copy) == NULL)
				{
					xmlFreeNode(copy);
					ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
							errmsg("could not append XML child node")));
				}
			}
		}
	}
}

static void
ivy_xml_addprevsiblingnode(xmlXPathObjectPtr xpathobj, const xmlChar* new_node)
{
	int			num = 0;
	int			i;
	xmlDocPtr	n_node = NULL;

	if (xpathobj->type == XPATH_NODESET)
	{
		if (xpathobj->nodesetval != NULL &&
			xpathobj->nodesetval->nodeTab != NULL)
		{
			num = xpathobj->nodesetval->nodeNr;
			for (i = 0; i < num; i++)
			{
				xmlNodePtr root;
				xmlNodePtr target = xpathobj->nodesetval->nodeTab[i];

				if (target == NULL)
					continue;
				if (target->type == XML_DOCUMENT_NODE ||
					target->type == XML_ATTRIBUTE_NODE ||
					target->type == XML_NAMESPACE_DECL ||
					target->parent == NULL)
					ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
							errmsg("invalid XPath expression")));

				n_node = ivy_xml_parse_document((const char *) new_node, xmlStrlen(new_node), 0);
				if (n_node == NULL)
					ereport(ERROR, (errcode(ERRCODE_INVALID_XML_DOCUMENT),
							 errmsg("could not parse XML new node")));
				root = xmlDocGetRootElement(n_node);
				if (root == NULL)
				{
					xmlFreeDoc(n_node);
					ereport(ERROR, (errcode(ERRCODE_INVALID_XML_DOCUMENT),
							errmsg("XML new node has no root element")));
				}
				/* Copy into the destination dictionary before freeing n_node. */
				root = xmlDocCopyNode(root, target->doc, 1);
				if (root == NULL)
				{
					xmlFreeDoc(n_node);
					ereport(ERROR, (errcode(ERRCODE_OUT_OF_MEMORY),
								errmsg("could not copy XML new node")));
				}
				if (xmlAddPrevSibling(target, root) == NULL)
				{
					xmlFreeNode(root);
					xmlFreeDoc(n_node);
					ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
							errmsg("could not insert XML sibling node")));
				}
				xmlFreeDoc(n_node);
			}
		}
	}

}

static void
ivy_xml_addnextsiblingnode(xmlXPathObjectPtr xpathobj, const xmlChar* new_node)
{
	int			num = 0;
	int			i;
	xmlDocPtr	n_node = NULL;

	if (xpathobj->type == XPATH_NODESET)
	{
		if (xpathobj->nodesetval != NULL &&
			xpathobj->nodesetval->nodeTab != NULL)
		{
			num = xpathobj->nodesetval->nodeNr;
			for (i = 0; i < num; i++)
			{
				xmlNodePtr root;
				xmlNodePtr target = xpathobj->nodesetval->nodeTab[i];

				if (target == NULL)
					continue;
				if (target->type == XML_DOCUMENT_NODE ||
					target->type == XML_ATTRIBUTE_NODE ||
					target->type == XML_NAMESPACE_DECL ||
					target->parent == NULL)
					ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
							errmsg("invalid XPath expression")));

				n_node = ivy_xml_parse_document((const char *) new_node, xmlStrlen(new_node), 0);
				if (n_node == NULL)
					ereport(ERROR, (errcode(ERRCODE_INVALID_XML_DOCUMENT),
							 errmsg("could not parse XML new node")));
				root = xmlDocGetRootElement(n_node);
				if (root == NULL)
				{
					xmlFreeDoc(n_node);
					ereport(ERROR, (errcode(ERRCODE_INVALID_XML_DOCUMENT),
							errmsg("XML new node has no root element")));
				}
				/* Copy into the destination dictionary before freeing n_node. */
				root = xmlDocCopyNode(root, target->doc, 1);
				if (root == NULL)
				{
					xmlFreeDoc(n_node);
					ereport(ERROR, (errcode(ERRCODE_OUT_OF_MEMORY),
								errmsg("could not copy XML new node")));
				}
				if (xmlAddNextSibling(target, root) == NULL)
				{
					xmlFreeNode(root);
					xmlFreeDoc(n_node);
					ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
							errmsg("could not insert XML sibling node")));
				}
				xmlFreeDoc(n_node);
			}
		}
	}

}

/*
 *Return false when Oracle document replacement produces SQL NULL.
 */
static bool
ivy_xml_replacenode(xpath_ws *ws, const xmlChar* new_node)
{
	xmlXPathObjectPtr xpathobj = ws->xpathobj;
	int			num = 0;
	int			i;
	xmlDocPtr	n_node = NULL;
	xmlNodePtr	cur = NULL;

	if (xpathobj->type == XPATH_NODESET)
	{
		if (xpathobj->nodesetval != NULL &&
			xpathobj->nodesetval->nodeTab != NULL)
		{
			num = xpathobj->nodesetval->nodeNr;
			/*
			 * XPath / selects the document, not its document element.  Replace
			 * the complete document (including comments and PIs), and release
			 * its XPath objects before freeing it.  Later update pairs must
			 * evaluate against the new document.  A document replacement also
			 * supersedes any descendant selected by the same expression.
			 */
			for (i = 0; i < num; i++)
			{
				cur = xpathobj->nodesetval->nodeTab[i];
				if (cur != NULL && cur->type == XML_DOCUMENT_NODE)
				{
					if (new_node[0] == '\0')
						continue;
					n_node = ivy_xml_parse_content((const char *) new_node,
												   xmlStrlen(new_node), 0);
					if (n_node == NULL)
					{
						cleanup_ws(ws);
						ereport(ERROR, (errcode(ERRCODE_INVALID_XML_DOCUMENT),
									errmsg("could not parse XML new node")));
					}
					/*
					 * Oracle returns SQL NULL when document replacement
					 * starts with a comment or PI.  The source's prolog does
					 * not matter.  Keep this rule separate from element
					 * replacement and from NULL replacement values.
					 */
					if (n_node->children != NULL &&
						(n_node->children->type == XML_COMMENT_NODE ||
						 n_node->children->type == XML_PI_NODE))
					{
						xmlFreeDoc(n_node);
						return false;
					}
					cleanup_ws(ws);
					ws->doc = n_node;
					return true;
				}
			}
			/* See ivy_xml_delenode: descendants must be processed first. */
			for (i = num - 1; i >= 0; i--)
			{
				cur = xpathobj->nodesetval->nodeTab[i];
				if (cur == NULL)
					continue;
				if (cur->type != XML_NAMESPACE_DECL)
					xpathobj->nodesetval->nodeTab[i] = NULL;
				if (strcmp((char *)new_node, "") == 0)
				{
					if (cur->type == XML_DOCUMENT_NODE)
						continue;
					if (cur->type == XML_TEXT_NODE || cur->type == XML_CDATA_SECTION_NODE)
					{
						xmlUnlinkNode(cur);
						xmlFreeNode(cur);
						continue;
					}
					if (cur->type != XML_ELEMENT_NODE)
						ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
								errmsg("invalid XPath expression")));
					if (cur->children != NULL)
					{
						xmlNodePtr child = cur->children;

						cur->children = NULL;
						cur->last = NULL;
						xmlFreeNodeList(child);
					}
				}
				else
				{
					xmlNodePtr root;
					xmlNodePtr old;

					n_node = ivy_xml_parse_document((const char *) new_node, xmlStrlen(new_node), 0);
					if (n_node == NULL)
						ereport(ERROR, (errcode(ERRCODE_INVALID_XML_DOCUMENT),
							errmsg("could not parse XML new node")));
					root = xmlDocGetRootElement(n_node);
					if (root == NULL)
					{
						xmlFreeDoc(n_node);
						ereport(ERROR, (errcode(ERRCODE_INVALID_XML_DOCUMENT),
								errmsg("XML new node has no root element")));
					}
					if (cur->type != XML_ELEMENT_NODE)
					{
						xmlFreeDoc(n_node);
						ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
								errmsg("invalid XPath expression")));
					}
					/* Copy into the destination dictionary before freeing n_node. */
					root = xmlDocCopyNode(root, cur->doc, 1);
					if (root == NULL)
					{
						xmlFreeDoc(n_node);
						ereport(ERROR, (errcode(ERRCODE_OUT_OF_MEMORY),
									errmsg("could not copy XML new node")));
					}
					old = xmlReplaceNode(cur, root);
					if (old == NULL)
					{
						xmlFreeNode(root);
						xmlFreeDoc(n_node);
						ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
								errmsg("could not replace XML node")));
					}
					xmlFreeNode(old);
					xmlFreeDoc(n_node);
				}
			}
		}
	}

	return true;
}

static xmlChar *
ivy_xmlCharStrndup(const char *str, size_t len)
{
	xmlChar    *result;

	result = (xmlChar *) palloc((len + 1) * sizeof(xmlChar));
	memcpy(result, str, len);
	result[len] = '\0';

	return result;
}
#endif		/* USE_LIBXML */


/*
 * xmltype IN function
 */
Datum
ivy_xmltype_in(PG_FUNCTION_ARGS)
{
#ifdef USE_LIBXML
	char	   *s = PG_GETARG_CSTRING(0);
	xmltype    *vardata;

	vardata = (xmltype *) cstring_to_text(s);

	PG_RETURN_XML_P(vardata);
#else
	NO_XML_SUPPORT();
	return 0;
#endif
}

/*
 * xmltype OUT function
 */
Datum
ivy_xmltype_out(PG_FUNCTION_ARGS)
{
	return xml_out(fcinfo);
}

//------------------------------------
// existsnode
//------------------------------------
Datum
ivy_existsnode(PG_FUNCTION_ARGS)
{
#ifdef USE_LIBXML
	int			res_nitems = 0;
	xmltype    *data = NULL;
	text	   *xpath_expr_text = NULL;
	xpath_ws   *ws = NULL;
	xmlXPathObjectPtr res;

	if (fcinfo->args[0].isnull)
		PG_RETURN_NULL();
	else
		data = PG_GETARG_XML_P(0);

	if (fcinfo->args[1].isnull)
		PG_RETURN_NULL();
	else
		xpath_expr_text = PG_GETARG_TEXT_PP(1);

	ws = ivy_xml_workspace();	/* BUG#Z9218 */

	res = ivy_xml_xpath(ws, xpath_expr_text, data, NULL);
	res_nitems = ivy_xpathobjnode_nr(res);

	cleanup_ws(ws);

	if (res_nitems > 1)
		res_nitems = 1;
	PG_RETURN_INT32(res_nitems);
#else
	NO_XML_SUPPORT();
	return 0;
#endif
}

Datum
ivy_existsnode2(PG_FUNCTION_ARGS)
{
#ifdef USE_LIBXML
	int			res_nitems = 0;
	xmltype    *data = NULL;
	char	   *cns = NULL;
	text	   *xpath_expr_text = NULL;
	text	   *ns = NULL;
	xpath_ws   *ws = NULL;
	xmlXPathObjectPtr res;

	if (fcinfo->args[0].isnull)
		PG_RETURN_NULL();
	else
		data = PG_GETARG_XML_P(0);

	if (fcinfo->args[1].isnull)
		PG_RETURN_NULL();
	else
		xpath_expr_text = PG_GETARG_TEXT_PP(1);

	if (!fcinfo->args[2].isnull)
	{
		ns = PG_GETARG_TEXT_PP(2);
		cns = text_to_cstring(ns);
	}

	ws = ivy_xml_workspace();	/* BUG#Z9218 */

	res = ivy_xml_xpath(ws, xpath_expr_text, data, cns);
	res_nitems = ivy_xpathobjnode_nr(res);

	cleanup_ws(ws);

	if (res_nitems > 1)
		res_nitems = 1;
	PG_RETURN_INT32(res_nitems);
#else
	NO_XML_SUPPORT();
	return 0;
#endif
}

//---------------------------------------
// extractvalue
//---------------------------------------
Datum
ivy_extractvalue(PG_FUNCTION_ARGS)
{
#ifdef USE_LIBXML
	xmltype    *data = NULL;
	text	   *xpath_expr_text = NULL;
	text	   *rval = NULL;
	char	   *ret = NULL;
	xpath_ws   *ws = NULL;
	xmlXPathObjectPtr res;

	StringInfoData result;

	if (fcinfo->args[0].isnull)
		PG_RETURN_NULL();
	else
		data = PG_GETARG_XML_P(0);

	if (fcinfo->args[1].isnull)
		PG_RETURN_NULL();
	else
		xpath_expr_text = PG_GETARG_TEXT_PP(1);

	initStringInfo(&result);
	ws = ivy_xml_workspace();	/* BUG#Z9218 */

	res = ivy_xml_xpath(ws, xpath_expr_text, data, NULL);

	if (res->nodesetval && res->nodesetval->nodeNr > 1)
	{
		cleanup_ws(ws);
		ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
				errmsg("EXTRACTVALUE returns value of only one node")));
	}

	/* Begin - Bug#Z203, Bug#Z214, Bug#z9075 */
	if (res->nodesetval && res->nodesetval->nodeNr > 0 &&
		res->nodesetval->nodeTab &&
		res->nodesetval->nodeTab[0] &&
		res->nodesetval->nodeTab[0]->type == XML_NAMESPACE_DECL)
	{
		cleanup_ws(ws);
		ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
				errmsg("invalid XPath expression")));
	}

	if (res->nodesetval && res->nodesetval->nodeNr > 0 &&
		res->nodesetval->nodeTab &&
		res->nodesetval->nodeTab[0] &&
		res->nodesetval->nodeTab[0]->parent == NULL)	/* BUG#Z7411, BUG#Z7412 */
	{
		cleanup_ws(ws);
		ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
				errmsg("EXTRACTVALUE can only retrieve value of leaf node")));
	}

	if (res->nodesetval && res->nodesetval->nodeNr > 0 &&
		res->nodesetval->nodeTab &&
		res->nodesetval->nodeTab[0] &&
		res->nodesetval->nodeTab[0]->children)
	{
		xmlNodePtr child;
		int elements = 0;

		for (child = res->nodesetval->nodeTab[0]->children; child; child = child->next)
			if (child->type == XML_ELEMENT_NODE)
				elements++;
		if (elements > 0)
		{
			cleanup_ws(ws);
			ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
					 errmsg("%s", elements == 1 ?
							"EXTRACTVALUE can only retrieve value of leaf node" :
							"EXTRACTVALUE returns value of only one node")));
		}
	}
	/* End - Bug#Z203, Bug#Z214, bug#9075 */

	ret = ivy_xml_xpathobjtostring(res);

	if (ret != NULL)
		appendStringInfoString(&result, ret);
	rval = cstring_to_text_with_len(result.data, result.len);
	pfree(result.data);

	cleanup_ws(ws);

	PG_RETURN_TEXT_P(rval);
#else
	NO_XML_SUPPORT();
	return 0;
#endif
}

Datum
ivy_extractvalue2(PG_FUNCTION_ARGS)
{
#ifdef USE_LIBXML
	xmltype    *data = NULL;
	text	   *xpath_expr_text = NULL;
	text	   *rval = NULL;
	text	   *ns = NULL;
	char	   *cns = NULL;
	char	   *ret = NULL;
	xpath_ws   *ws = NULL;
	xmlXPathObjectPtr res;

	StringInfoData result;

	if (fcinfo->args[0].isnull)
		PG_RETURN_NULL();
	else
		data = PG_GETARG_XML_P(0);

	if (fcinfo->args[1].isnull)
		PG_RETURN_NULL();
	else
		xpath_expr_text = PG_GETARG_TEXT_PP(1);

	if (!fcinfo->args[2].isnull)
	{
		ns = PG_GETARG_TEXT_PP(2);
		cns = text_to_cstring(ns);
	}

	initStringInfo(&result);
	ws = ivy_xml_workspace();	/* BUG#Z9218 */

	res = ivy_xml_xpath(ws, xpath_expr_text, data, cns);

	if (res->nodesetval && res->nodesetval->nodeNr > 1)
	{
		cleanup_ws(ws);
		ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
				errmsg("EXTRACTVALUE returns value of only one node")));
	}

	/* Begin - Bug#Z203, Bug#Z214, bug#9075 */
	if (res->nodesetval && res->nodesetval->nodeNr > 0 &&
		res->nodesetval->nodeTab &&
		res->nodesetval->nodeTab[0] &&
		res->nodesetval->nodeTab[0]->type == XML_NAMESPACE_DECL)
	{
		cleanup_ws(ws);
		ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
				errmsg("invalid XPath expression")));
	}

	if (res->nodesetval && res->nodesetval->nodeNr > 0 &&
		res->nodesetval->nodeTab &&
		res->nodesetval->nodeTab[0] &&
		res->nodesetval->nodeTab[0]->parent == NULL)	/* BUG#Z7411, BUG#Z7412 */
	{
		cleanup_ws(ws);
		ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
				errmsg("EXTRACTVALUE can only retrieve value of leaf node")));
	}

	if (res->nodesetval && res->nodesetval->nodeNr > 0 &&
		res->nodesetval->nodeTab &&
		res->nodesetval->nodeTab[0] &&
		res->nodesetval->nodeTab[0]->children)
	{
		xmlNodePtr child;
		int elements = 0;

		for (child = res->nodesetval->nodeTab[0]->children; child; child = child->next)
			if (child->type == XML_ELEMENT_NODE)
				elements++;
		if (elements > 0)
		{
			cleanup_ws(ws);
			ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
					 errmsg("%s", elements == 1 ?
							"EXTRACTVALUE can only retrieve value of leaf node" :
							"EXTRACTVALUE returns value of only one node")));
		}
	}
	/* End - Bug#Z203, Bug#Z214, bug#9075*/

	ret = ivy_xml_xpathobjtostring(res);

	if (ret != NULL)
		appendStringInfoString(&result, ret);
	rval = cstring_to_text_with_len(result.data, result.len);
	pfree(result.data);

	cleanup_ws(ws);

	PG_RETURN_TEXT_P(rval);
#else
	NO_XML_SUPPORT();
	return 0;
#endif
}

//--------------------------------
// extract
//--------------------------------
Datum
ivy_extract(PG_FUNCTION_ARGS)
{
#ifdef USE_LIBXML
	xmltype    *data = NULL;
	xmltype    *ret = NULL;
	text	   *xpath_expr_text = NULL;
	text	   *rval = NULL;
	xpath_ws   *ws = NULL;
	xmlXPathObjectPtr res;

	StringInfoData result;

	if (fcinfo->args[0].isnull)
		PG_RETURN_NULL();
	else
		data = PG_GETARG_XML_P(0);

	if (fcinfo->args[1].isnull)
		PG_RETURN_NULL();
	else
		xpath_expr_text = PG_GETARG_TEXT_PP(1);

	initStringInfo(&result);
	ws = ivy_xml_workspace();	/* BUG#Z9218 */

	res = ivy_xml_xpath(ws, xpath_expr_text, data, NULL);
	ret = ivy_xml_xpathobjtoxmltype(res);

	cleanup_ws(ws);

	if (ret)
	{
		ret = (xmltype *)rv_newline((text *)ret);
		PG_RETURN_XML_P(ret);
	}
	else
	{
		appendStringInfoString(&result, "");
		rval = cstring_to_text_with_len(result.data, result.len);
		pfree(result.data);

		PG_RETURN_TEXT_P(rval);
	}
#else
	NO_XML_SUPPORT();
	return 0;
#endif
}

Datum
ivy_extract2(PG_FUNCTION_ARGS)
{
#ifdef USE_LIBXML
	xmltype    *data = NULL;
	xmltype    *ret = NULL;
	text	   *xpath_expr_text = NULL;
	text	   *rval = NULL;
	text	   *ns = NULL;
	char	   *cns = NULL;
	xpath_ws   *ws = NULL;
	xmlXPathObjectPtr res;

	StringInfoData result;

	if (fcinfo->args[0].isnull)
		PG_RETURN_NULL();
	else
		data = PG_GETARG_XML_P(0);

	if (fcinfo->args[1].isnull)
		PG_RETURN_NULL();
	else
		xpath_expr_text = PG_GETARG_TEXT_PP(1);

	if (!fcinfo->args[2].isnull)
	{
		ns = PG_GETARG_TEXT_PP(2);
		cns = text_to_cstring(ns);
	}

	initStringInfo(&result);
	ws = ivy_xml_workspace();	/* BUG#Z9218 */

	res = ivy_xml_xpath(ws, xpath_expr_text, data, cns);
	ret = ivy_xml_xpathobjtoxmltype(res);

	cleanup_ws(ws);

	if (ret)
	{
		ret = (xmltype *)rv_newline((text *)ret);
		PG_RETURN_XML_P(ret);
	}
	else
	{
		appendStringInfoString(&result, "");
		rval = cstring_to_text_with_len(result.data, result.len);
		pfree(result.data);

		PG_RETURN_TEXT_P(rval);
	}
#else
	NO_XML_SUPPORT();
	return 0;
#endif
}

//-----------------------------------
// deletexml
//-----------------------------------
Datum
ivy_deletexml(PG_FUNCTION_ARGS)
{
#ifdef USE_LIBXML
	xmltype    *data = NULL;
	text	   *xpath_expr_text = NULL;
	xmltype    *ret = NULL;
	xpath_ws   *ws = NULL;
	xmlXPathObjectPtr res;

	if (fcinfo->args[0].isnull)
		PG_RETURN_NULL();
	else
		data = PG_GETARG_XML_P(0);

	if (fcinfo->args[1].isnull)
		PG_RETURN_NULL();
	else
	{
		xpath_expr_text = PG_GETARG_TEXT_PP(1);
	}

	ws = ivy_xml_workspace();	/* BUG#Z9218 */

	res = ivy_xml_xpath(ws, xpath_expr_text, data, NULL);
	if (res->nodesetval && res->nodesetval->nodeTab != NULL)
		ivy_xml_delenode(res);

	ret = (xmltype *)ivy_xml_xmlnode2xmltype((xmlNodePtr)ws->doc);
	ret = (xmltype *)rv_newline((text *)ret);

	cleanup_ws(ws);

	PG_RETURN_XML_P(ret);
#else
	NO_XML_SUPPORT();
	return 0;
#endif
}

Datum
ivy_deletexml2(PG_FUNCTION_ARGS)
{
#ifdef USE_LIBXML
	xmltype    *data = NULL;
	text	   *xpath_expr_text = NULL;
	text	   *ns = NULL;
	xmltype    *ret = NULL;
	char	   *cns= NULL;
	xpath_ws   *ws = NULL;
	xmlXPathObjectPtr res;

	if (fcinfo->args[0].isnull)
		PG_RETURN_NULL();
	else
		data = PG_GETARG_XML_P(0);

	if (fcinfo->args[1].isnull)
		PG_RETURN_NULL();
	else
	{
		xpath_expr_text = PG_GETARG_TEXT_PP(1);
	}

	if (!fcinfo->args[2].isnull)
	{
		ns = PG_GETARG_TEXT_PP(2);
		cns = text_to_cstring(ns);
	}

	ws = ivy_xml_workspace();	/* BUG#Z9218 */

	res = ivy_xml_xpath(ws, xpath_expr_text, data, cns);
	if (res->nodesetval && res->nodesetval->nodeTab != NULL)
		ivy_xml_delenode(res);

	ret = (xmltype *)ivy_xml_xmlnode2xmltype((xmlNodePtr)ws->doc);
	ret = (xmltype *)rv_newline((text *)ret);

	cleanup_ws(ws);

	PG_RETURN_XML_P(ret);
#else
	NO_XML_SUPPORT();
	return 0;
#endif
}

//-----------------------------------------
// appendchildxml
//-----------------------------------------
Datum
ivy_appendchildxml(PG_FUNCTION_ARGS)
{
#ifdef USE_LIBXML
	xmltype    *data = NULL;
	text	   *xpath_expr_text = NULL;
	char	   *xpsth_csting = NULL;	/* BUG#Z7412 */
	xmltype    *var = NULL;
	xmltype    *ret = NULL;
	char	   *cnodestr = NULL;
	int 		len;
	xpath_ws   *ws = NULL;
	xmlDocPtr 	new_node = NULL;
	xmlChar	   *nodestring = NULL;
	xmlXPathObjectPtr res;

	if (fcinfo->args[0].isnull)
		PG_RETURN_NULL();
	else
		data = PG_GETARG_XML_P(0);

	if (fcinfo->args[1].isnull)
		PG_RETURN_NULL();
	else
	{
		xpath_expr_text = PG_GETARG_TEXT_PP(1);
		xpsth_csting = ivy_trim(text_to_cstring(xpath_expr_text));	/* BUG#Z7412 */
	}

	/* Begin - BUG#Z7412 */
	if (strcmp(xpsth_csting, "/") == 0)
		ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
				errmsg("Invalid XPATH expression")));
	/* End - BUG#Z7412 */

	if (!fcinfo->args[2].isnull)
	{
		var = PG_GETARG_XML_P(2);
		cnodestr = VARDATA(var);
		len = VARSIZE(var) - VARHDRSZ;
		nodestring = ivy_xmlCharStrndup(cnodestr, len);
	}
	else
		PG_RETURN_XML_P(data);

	ws = ivy_xml_workspace();	/* BUG#Z9218 */

	res = ivy_xml_xpath(ws, xpath_expr_text, data, NULL);

	ws->tempdoc = new_node = ivy_xml_parse_document((const char *) nodestring, xmlStrlen(nodestring), 0);
	if (new_node == NULL)
	{
		cleanup_ws(ws);
		ereport(ERROR, (errcode(ERRCODE_INVALID_XML_DOCUMENT),
				errmsg("could not parse XML new node")));
	}

	ivy_xml_addchildnode(res, (xmlNodePtr)new_node, false);
	ret = (xmltype *)ivy_xml_xmlnode2xmltype((xmlNodePtr)ws->doc);

	ret = (xmltype *)rv_newline((text *)ret);

	ivy_xml_free_tempdoc(ws);

	cleanup_ws(ws);

	PG_RETURN_XML_P(ret);
#else
	NO_XML_SUPPORT();
	return 0;
#endif
}

Datum
ivy_appendchildxml2(PG_FUNCTION_ARGS)
{
#ifdef USE_LIBXML
	xmltype    *data = NULL;
	text	   *xpath_expr_text = NULL;
	char	   *xpsth_csting = NULL;	/* BUG#Z7412 */
	xmltype    *var = NULL;
	text	   *ns = NULL;
	xmltype    *ret = NULL;
	char	   *cns = NULL;
	char	   *cnodestr = NULL;
	int 		len;
	xpath_ws   *ws = NULL;
	xmlDocPtr 	new_node = NULL;
	xmlChar	   *nodestring = NULL;
	xmlXPathObjectPtr res;

	if (fcinfo->args[0].isnull)
		PG_RETURN_NULL();
	else
		data = PG_GETARG_XML_P(0);

	if (fcinfo->args[1].isnull)
		PG_RETURN_NULL();
	else
	{
		xpath_expr_text = PG_GETARG_TEXT_PP(1);
		xpsth_csting = ivy_trim(text_to_cstring(xpath_expr_text));	/* BUG#Z7412 */
	}

	/* Begin - BUG#Z7412 */
	if (strcmp(xpsth_csting, "/") == 0)
		ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
				errmsg("Invalid XPATH expression")));
	/* End - BUG#Z7412 */

	if (!fcinfo->args[2].isnull)
	{
		var = PG_GETARG_XML_P(2);
		cnodestr = VARDATA(var);
		len = VARSIZE(var) - VARHDRSZ;
		nodestring = ivy_xmlCharStrndup(cnodestr, len);
	}
	else
		PG_RETURN_XML_P(data);

	if (!fcinfo->args[3].isnull)
	{
		ns = PG_GETARG_TEXT_PP(3);
		cns = text_to_cstring(ns);
	}

	ws = ivy_xml_workspace();	/* BUG#Z9218 */

	res = ivy_xml_xpath(ws, xpath_expr_text, data, cns);

	ws->tempdoc = new_node = ivy_xml_parse_document((const char *) nodestring, xmlStrlen(nodestring), 0);
	if (new_node == NULL)
	{
		cleanup_ws(ws);
		ereport(ERROR, (errcode(ERRCODE_INVALID_XML_DOCUMENT),
				errmsg("could not parse XML new node")));
	}

	ivy_xml_addchildnode(res, (xmlNodePtr)new_node, false);
	ret = (xmltype *)ivy_xml_xmlnode2xmltype((xmlNodePtr)ws->doc);

	ret = (xmltype *)rv_newline((text *)ret);

	ivy_xml_free_tempdoc(ws);

	cleanup_ws(ws);

	PG_RETURN_XML_P(ret);
#else
	NO_XML_SUPPORT();
	return 0;
#endif
}

//-------------------------------------------
// insertxmlbefore
//-------------------------------------------
Datum
ivy_insertxmlbefore(PG_FUNCTION_ARGS)
{
#ifdef USE_LIBXML
	xmltype    *data = NULL;
	text	   *xpath_expr_text = NULL;
	char	   *xpsth_csting = NULL;	/* BUG#Z7412 */
	xmltype    *var = NULL;
	xmltype    *ret;
	char	   *cnodestr = NULL;
	int 		len;
	xpath_ws   *ws = NULL;
	xmlChar	   *nodestring = NULL;
	xmlXPathObjectPtr res;

	if (fcinfo->args[0].isnull)
		PG_RETURN_NULL();
	else
		data = PG_GETARG_XML_P(0);

	if (fcinfo->args[1].isnull)
		PG_RETURN_NULL();
	else
	{
		xpath_expr_text = PG_GETARG_TEXT_PP(1);
		xpsth_csting = ivy_trim(text_to_cstring(xpath_expr_text));	/* BUG#Z7412 */
	}

	/* Begin - BUG#Z7412 */
	if (strcmp(xpsth_csting, "/") == 0)
		ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
				errmsg("Invalid XPATH expression")));
	/* End - BUG#Z7412 */

	if (!fcinfo->args[2].isnull)
	{
		var = PG_GETARG_XML_P(2);
		cnodestr = VARDATA(var);
		len = VARSIZE(var) - VARHDRSZ;
		nodestring = ivy_xmlCharStrndup(cnodestr, len);
	}
	else
		PG_RETURN_XML_P(data);

	ws = ivy_xml_workspace();	/* BUG#Z9218 */

	res = ivy_xml_xpath(ws, xpath_expr_text, data, NULL);

	if (res->nodesetval && res->nodesetval->nodeNr == 1 && /* Bug#Z205 */
		res->nodesetval->nodeTab && res->nodesetval->nodeTab[0] &&
		(res->nodesetval->nodeTab[0]->type == XML_NAMESPACE_DECL ||
		 res->nodesetval->nodeTab[0]->parent == NULL ||
		 res->nodesetval->nodeTab[0]->parent->type == XML_DOCUMENT_NODE))
	{
		cleanup_ws(ws);
		ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
				errmsg("invalid XPath expression")));
	}

	ivy_xml_addprevsiblingnode(res, nodestring);
	ret = (xmltype *)ivy_xml_xmlnode2xmltype((xmlNodePtr)ws->doc);
	ret = (xmltype *)rv_newline((text *)ret);

	cleanup_ws(ws);

	PG_RETURN_XML_P(ret);
#else
	NO_XML_SUPPORT();
	return 0;
#endif
}

Datum
ivy_insertxmlbefore2(PG_FUNCTION_ARGS)
{
#ifdef USE_LIBXML
	xmltype    *data = NULL;
	text	   *xpath_expr_text = NULL;
	char	   *xpsth_csting = NULL;	/* BUG#Z7412 */
	xmltype    *var = NULL;
	text	   *ns = NULL;
	xmltype    *ret;
	char	   *cns = NULL;
	char	   *cnodestr = NULL;
	int 		len;
	xpath_ws   *ws = NULL;
	xmlChar	   *nodestring = NULL;
	xmlXPathObjectPtr res;

	if (fcinfo->args[0].isnull)
		PG_RETURN_NULL();
	else
		data = PG_GETARG_XML_P(0);

	if (fcinfo->args[1].isnull)
		PG_RETURN_NULL();
	else
	{
		xpath_expr_text = PG_GETARG_TEXT_PP(1);
		xpsth_csting = ivy_trim(text_to_cstring(xpath_expr_text));	/* BUG#Z7412 */
	}

	/* Begin - BUG#Z7412 */
	if (strcmp(xpsth_csting, "/") == 0)
		ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
				errmsg("Invalid XPATH expression")));
	/* End - BUG#Z7412 */

	if (!fcinfo->args[2].isnull)
	{
		var = PG_GETARG_XML_P(2);
		cnodestr = VARDATA(var);
		len = VARSIZE(var) - VARHDRSZ;
		nodestring = ivy_xmlCharStrndup(cnodestr, len);
	}
	else
		PG_RETURN_XML_P(data);

	if (!fcinfo->args[3].isnull)
	{
		ns = PG_GETARG_TEXT_PP(3);
		cns = text_to_cstring(ns);
	}

	ws = ivy_xml_workspace();	/* BUG#Z9218 */

	res = ivy_xml_xpath(ws, xpath_expr_text, data, cns);

	if (res->nodesetval && res->nodesetval->nodeNr == 1 && /* Bug#Z205 */
		res->nodesetval->nodeTab && res->nodesetval->nodeTab[0] &&
		(res->nodesetval->nodeTab[0]->type == XML_NAMESPACE_DECL ||
		 res->nodesetval->nodeTab[0]->parent == NULL ||
		 res->nodesetval->nodeTab[0]->parent->type == XML_DOCUMENT_NODE))
	{
		cleanup_ws(ws);
		ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
				errmsg("invalid XPath expression")));
	}

	ivy_xml_addprevsiblingnode(res, nodestring);
	ret = (xmltype *)ivy_xml_xmlnode2xmltype((xmlNodePtr)ws->doc);
	ret = (xmltype *)rv_newline((text *)ret);

	cleanup_ws(ws);

	PG_RETURN_XML_P(ret);
#else
	NO_XML_SUPPORT();
	return 0;
#endif
}

//------------------------------------------
// insertxmlafter
//------------------------------------------
Datum
ivy_insertxmlafter(PG_FUNCTION_ARGS)
{
#ifdef USE_LIBXML
	xmltype    *data= NULL;
	text	   *xpath_expr_text = NULL;
	char	   *xpsth_csting = NULL;	/* BUG#Z7412 */
	xmltype    *var = NULL;
	xmltype    *ret;
	char	   *cnodestr = NULL;
	int 		len;
	xpath_ws   *ws = NULL;
	xmlChar	   *nodestring = NULL;
	xmlXPathObjectPtr res;

	if (fcinfo->args[0].isnull)
		PG_RETURN_NULL();
	else
		data = PG_GETARG_XML_P(0);

	if (fcinfo->args[1].isnull)
		PG_RETURN_NULL();
	else
	{
		xpath_expr_text = PG_GETARG_TEXT_PP(1);
		xpsth_csting = ivy_trim(text_to_cstring(xpath_expr_text));	/* BUG#Z7412 */
	}

	/* Begin - BUG#Z7412 */
	if (strcmp(xpsth_csting, "/") == 0)
		ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
				errmsg("Invalid XPATH expression")));
	/* End - BUG#Z7412 */

	if (!fcinfo->args[2].isnull)
	{
		var = PG_GETARG_XML_P(2);
		cnodestr = VARDATA(var);
		len = VARSIZE(var) - VARHDRSZ;
		nodestring = ivy_xmlCharStrndup(cnodestr, len);
	}
	else
		PG_RETURN_XML_P(data);

	ws = ivy_xml_workspace();	/* BUG#Z9218 */

	res = ivy_xml_xpath(ws, xpath_expr_text, data, NULL);

	ivy_xml_addnextsiblingnode(res, nodestring);
	ret = (xmltype *)ivy_xml_xmlnode2xmltype((xmlNodePtr)ws->doc);
	ret = (xmltype *)rv_newline((text *)ret);

	cleanup_ws(ws);

	PG_RETURN_XML_P(ret);
#else
	NO_XML_SUPPORT();
	return 0;
#endif
}

Datum
ivy_insertxmlafter2(PG_FUNCTION_ARGS)
{
#ifdef USE_LIBXML
	xmltype    *data = NULL;
	text	   *xpath_expr_text = NULL;
	char	   *xpsth_csting = NULL;	/* BUG#Z7412 */
	xmltype    *var = NULL;
	text	   *ns = NULL;
	xmltype    *ret = NULL;
	char	   *cns = NULL;
	char	   *cnodestr = NULL;
	int 		len;
	xpath_ws   *ws = NULL;
	xmlChar	   *nodestring = NULL;
	xmlXPathObjectPtr res;

	if (fcinfo->args[0].isnull)
		PG_RETURN_NULL();
	else
		data = PG_GETARG_XML_P(0);

	if (fcinfo->args[1].isnull)
		PG_RETURN_NULL();
	else
	{
		xpath_expr_text = PG_GETARG_TEXT_PP(1);
		xpsth_csting = ivy_trim(text_to_cstring(xpath_expr_text));	/* BUG#Z7412 */
	}

	/* Begin - BUG#Z7412 */
	if (strcmp(xpsth_csting, "/") == 0)
		ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
				errmsg("Invalid XPATH expression")));
	/* End - BUG#Z7412 */

	if (!fcinfo->args[2].isnull)
	{
		var = PG_GETARG_XML_P(2);
		cnodestr = VARDATA(var);
		len = VARSIZE(var) - VARHDRSZ;
		nodestring = ivy_xmlCharStrndup(cnodestr, len);
	}
	else
		PG_RETURN_XML_P(data);

	if (!fcinfo->args[3].isnull)
	{
		ns = PG_GETARG_TEXT_PP(3);
		cns = text_to_cstring(ns);
	}

	ws = ivy_xml_workspace();	/* BUG#Z9218 */

	res = ivy_xml_xpath(ws, xpath_expr_text, data, cns);

	ivy_xml_addnextsiblingnode(res, nodestring);
	ret = (xmltype *)ivy_xml_xmlnode2xmltype((xmlNodePtr)ws->doc);
	ret = (xmltype *)rv_newline((text *)ret);

	cleanup_ws(ws);

	PG_RETURN_XML_P(ret);
#else
	NO_XML_SUPPORT();
	return 0;
#endif
}

//------------------------------------------
// insertchildxml
//------------------------------------------
Datum
ivy_insertchildxml(PG_FUNCTION_ARGS)
{
#ifdef USE_LIBXML
	xmltype    *data = NULL;
	text	   *xpath_expr_text = NULL;
	text	   *n_name = NULL;
	xmltype    *var = NULL;
	xmltype    *ret = NULL;
	char	   *cname = NULL;
	char	   *cnodestr = NULL;
	int 		len;
	xpath_ws   *ws = NULL;
	xmlDocPtr 	new_node = NULL;
	xmlNodePtr	new_root = NULL;
	xmlChar	   *nodestring = NULL;
	xmlXPathObjectPtr res;

	if (fcinfo->args[0].isnull)
		PG_RETURN_NULL();
	else
		data = PG_GETARG_XML_P(0);

	if (fcinfo->args[1].isnull)
		PG_RETURN_NULL();
	else
	{
		xpath_expr_text = PG_GETARG_TEXT_PP(1);
	}

	if (!fcinfo->args[2].isnull)
	{
		n_name = PG_GETARG_TEXT_PP(2);
		cname = text_to_cstring(n_name);
	}
	else
		ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
				 errmsg("The document being inserted does not conform to specified child name")));

	if (!fcinfo->args[3].isnull)
	{
		var = PG_GETARG_XML_P(3);
		cnodestr = VARDATA(var);
		len = VARSIZE(var) - VARHDRSZ;
		nodestring = ivy_xmlCharStrndup(cnodestr, len);
	}
	else
		ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
				errmsg("invalid XPath expression")));

	ws = ivy_xml_workspace();	/* BUG#Z9218 */
	ws->tempdoc = new_node = ivy_xml_parse_document((const char *) nodestring, xmlStrlen(nodestring), 0);
	if (new_node == NULL)
	{
		ereport(ERROR, (errcode(ERRCODE_INVALID_XML_DOCUMENT),
				errmsg("could not parse XML new node")));
	}
	else
	{
		new_root = xmlDocGetRootElement(new_node);
		if (new_root == NULL)
		{
			ivy_xml_free_tempdoc(ws);
			ereport(ERROR, (errcode(ERRCODE_INVALID_XML_DOCUMENT),
					errmsg("XML new node has no root element")));
		}
		/* Begin - Bug#Z204 */
		if (strcmp(cname, (char *)new_root->name) != 0)
		{
			ivy_xml_free_tempdoc(ws);
			ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
					errmsg("The document being inserted does not conform to specified child name")));
		}
		/* End - Bug#Z204 */
	}

	res = ivy_xml_xpath(ws, xpath_expr_text, data, NULL);
	ivy_xml_addchildnode(res, (xmlNodePtr)new_node, true);
	ret = (xmltype *)ivy_xml_xmlnode2xmltype((xmlNodePtr)ws->doc);
	ret = (xmltype *)rv_newline((text *)ret);

	ivy_xml_free_tempdoc(ws);

	cleanup_ws(ws);

	PG_RETURN_XML_P(ret);
#else
	NO_XML_SUPPORT();
	return 0;
#endif
}

Datum
ivy_insertchildxml2(PG_FUNCTION_ARGS)
{
#ifdef USE_LIBXML
	xmltype    *data = NULL;
	text	   *xpath_expr_text = NULL;
	text	   *n_name = NULL;
	xmltype    *var = NULL;
	text	   *ns = NULL;
	xmltype    *ret = NULL;
	char	   *cname = NULL;
	char	   *cns = NULL;
	char	   *cnodestr = NULL;
	int 		len;
	xpath_ws   *ws = NULL;
	xmlDocPtr 	new_node = NULL;
	xmlNodePtr	new_root = NULL;
	xmlChar	   *nodestring = NULL;
	xmlXPathObjectPtr res;

	StringInfoData r;

	if (fcinfo->args[0].isnull)
		PG_RETURN_NULL();
	else
		data = PG_GETARG_XML_P(0);

	if (fcinfo->args[1].isnull)
		PG_RETURN_NULL();
	else
	{
		xpath_expr_text = PG_GETARG_TEXT_PP(1);
	}

	if (!fcinfo->args[2].isnull)
	{
		n_name = PG_GETARG_TEXT_PP(2);
		cname = text_to_cstring(n_name);
	}
	else
		ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
				 errmsg("The document being inserted does not conform to specified child name")));

	if (!fcinfo->args[3].isnull)
	{
		var = PG_GETARG_XML_P(3);
		cnodestr = VARDATA(var);
		len = VARSIZE(var) - VARHDRSZ;
		nodestring = ivy_xmlCharStrndup(cnodestr, len);
	}
	else
		ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
				errmsg("invalid XPath expression")));

	if (!fcinfo->args[4].isnull)
	{
		ns = PG_GETARG_TEXT_PP(4);
		cns = text_to_cstring(ns);
	}

	ws = ivy_xml_workspace();	/* BUG#Z9218 */
	ws->tempdoc = new_node = ivy_xml_parse_document((const char *) nodestring, xmlStrlen(nodestring), 0);
	if (new_node == NULL)
	{
		ereport(ERROR, (errcode(ERRCODE_INVALID_XML_DOCUMENT),
				errmsg("could not parse XML new node")));
	}
	else
	{
		new_root = xmlDocGetRootElement(new_node);
		if (new_root == NULL)
		{
			ivy_xml_free_tempdoc(ws);
			ereport(ERROR, (errcode(ERRCODE_INVALID_XML_DOCUMENT),
					errmsg("XML new node has no root element")));
		}
		/* Begin - Bug#Z204, Bug#Z215 */
		initStringInfo(&r);
		appendStringInfoString(&r, "<");
		appendStringInfoString(&r, cname);

		if (strstr((char *)nodestring, r.data))
		{
			if (!strstr(cname, (char *)new_root->name))
			{
				ivy_xml_free_tempdoc(ws);
				ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
						errmsg("The document being inserted does not conform to specified child name")));
			}
		}
		else
		{
			ivy_xml_free_tempdoc(ws);
			ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
					errmsg("The document being inserted does not conform to specified child name")));
		}
		/* End - Bug#Z204, Bug#Z215 */
	}

	res = ivy_xml_xpath(ws, xpath_expr_text, data, cns);
	ivy_xml_addchildnode(res, (xmlNodePtr)new_node, true);
	ret = (xmltype *)ivy_xml_xmlnode2xmltype((xmlNodePtr)ws->doc);
	ret = (xmltype *)rv_newline((text *)ret);

	ivy_xml_free_tempdoc(ws);

	cleanup_ws(ws);

	PG_RETURN_XML_P(ret);
#else
	NO_XML_SUPPORT();
	return 0;
#endif
}

//--------------------------------------------------
// insertchildxmlbefore
//--------------------------------------------------
Datum
ivy_insertchildxmlbefore(PG_FUNCTION_ARGS)
{
#ifdef USE_LIBXML
	xmltype    *data = NULL;
	text	   *xpath_expr_text = NULL;
	text	   *child_expr_text = NULL;
	xmltype    *var = NULL;
	char	   *c_expr = NULL;
	char	   *xpath_expr = NULL;
	xmltype    *ret = NULL;
	text	   *full_path = NULL;
	char	   *cnodestr = NULL;
	int 		len;
	xpath_ws   *ws = NULL;
	xmlChar	   *nodestring = NULL;
	xmlXPathObjectPtr res;

	StringInfoData result;

	if (fcinfo->args[0].isnull)
		PG_RETURN_NULL();
	else
		data = PG_GETARG_XML_P(0);

	if (fcinfo->args[1].isnull)
		PG_RETURN_NULL();
	else
	{
		xpath_expr_text = PG_GETARG_TEXT_PP(1);
		xpath_expr = text_to_cstring(xpath_expr_text);
	}

	if (!fcinfo->args[2].isnull)
	{
		child_expr_text = PG_GETARG_TEXT_PP(2);
		c_expr = text_to_cstring(child_expr_text);
	}
	else
		ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
				 errmsg("The document being inserted does not conform to specified child name")));

	if (!fcinfo->args[3].isnull)
	{
		var = PG_GETARG_XML_P(3);
		cnodestr = VARDATA(var);
		len = VARSIZE(var) - VARHDRSZ;
		nodestring = ivy_xmlCharStrndup(cnodestr, len);
	}
	else
		 PG_RETURN_XML_P(data);

	ivy_xml_check_child_path(xpath_expr);

	initStringInfo(&result);
	appendStringInfoString(&result, xpath_expr);
	appendStringInfoString(&result, "/");
	appendStringInfoString(&result, c_expr);

	full_path = cstring_to_text(result.data);
	pfree(result.data);

	ws = ivy_xml_workspace();	/* BUG#Z9218 */

	res = ivy_xml_xpath(ws, full_path, data, NULL);
	ivy_xml_addprevsiblingnode(res, nodestring);
	ret = (xmltype *)ivy_xml_xmlnode2xmltype((xmlNodePtr)ws->doc);
	ret = (xmltype *)rv_newline((text *)ret);

	cleanup_ws(ws);

	PG_RETURN_XML_P(ret);
#else
	NO_XML_SUPPORT();
	return 0;
#endif
}

Datum
ivy_insertchildxmlbefore2(PG_FUNCTION_ARGS)
{
#ifdef USE_LIBXML
	xmltype    *data = NULL;
	text	   *xpath_expr_text = NULL;
	text	   *child_expr_text = NULL;
	xmltype    *var = NULL;
	text	   *ns = NULL;
	xmltype    *ret = NULL;
	char	   *c_expr = NULL;
	char	   *xpath_expr = NULL;
	char	   *cns = NULL;
	text	   *full_path = NULL;
	char	   *cnodestr = NULL;
	int 		len;
	xpath_ws   *ws = NULL;
	xmlChar	   *nodestring = NULL;
	xmlXPathObjectPtr res;

	StringInfoData result;

	if (fcinfo->args[0].isnull)
		PG_RETURN_NULL();
	else
		data = PG_GETARG_XML_P(0);

	if (fcinfo->args[1].isnull)
		PG_RETURN_NULL();
	else
	{
		xpath_expr_text = PG_GETARG_TEXT_PP(1);
		xpath_expr = text_to_cstring(xpath_expr_text);
	}

	if (!fcinfo->args[2].isnull)
	{
		child_expr_text = PG_GETARG_TEXT_PP(2);
		c_expr = text_to_cstring(child_expr_text);
	}
	else
		ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
				errmsg("The document being inserted does not conform to specified child name")));

	if (!fcinfo->args[3].isnull)
	{
		var = PG_GETARG_XML_P(3);
		cnodestr = VARDATA(var);
		len = VARSIZE(var) - VARHDRSZ;
		nodestring = ivy_xmlCharStrndup(cnodestr, len);
	}
	else
		PG_RETURN_XML_P(data);

	if (!fcinfo->args[4].isnull)
	{
		ns = PG_GETARG_TEXT_PP(4);
		cns = text_to_cstring(ns);
	}

	ivy_xml_check_child_path(xpath_expr);

	initStringInfo(&result);
	appendStringInfoString(&result, xpath_expr);
	appendStringInfoString(&result, "/");
	appendStringInfoString(&result, c_expr);

	full_path = cstring_to_text(result.data);
	pfree(result.data);

	ws = ivy_xml_workspace();	/* BUG#Z9218 */

	res = ivy_xml_xpath(ws, full_path, data, cns);
	ivy_xml_addprevsiblingnode(res, nodestring);
	ret = (xmltype *)ivy_xml_xmlnode2xmltype((xmlNodePtr)ws->doc);
	ret = (xmltype *)rv_newline((text *)ret);

	cleanup_ws(ws);

	PG_RETURN_XML_P(ret);
#else
	NO_XML_SUPPORT();
	return 0;
#endif
}

//-------------------------------------------------
// insertchildxmlafter
//-------------------------------------------------
Datum
ivy_insertchildxmlafter(PG_FUNCTION_ARGS)
{
#ifdef USE_LIBXML
	xmltype    *data = NULL;
	text	   *xpath_expr_text = NULL;
	text	   *child_expr_text = NULL;
	xmltype    *var= NULL;
	xmltype    *ret = NULL;
	char	   *c_expr = NULL;
	char	   *xpath_expr = NULL;
	text	   *full_path = NULL;
	char	   *cnodestr = NULL;
	int 		len;
	xpath_ws   *ws = NULL;
	xmlChar	   *nodestring = NULL;
	xmlXPathObjectPtr res;

	StringInfoData result;

	if (fcinfo->args[0].isnull)
		PG_RETURN_NULL();
	else
		data = PG_GETARG_XML_P(0);

	if (fcinfo->args[1].isnull)
		PG_RETURN_NULL();
	else
	{
		xpath_expr_text = PG_GETARG_TEXT_PP(1);
		xpath_expr = text_to_cstring(xpath_expr_text);
	}

	if (!fcinfo->args[2].isnull)
	{
		child_expr_text = PG_GETARG_TEXT_PP(2);
		c_expr = text_to_cstring(child_expr_text);
	}
	else
		ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
				 errmsg("The document being inserted does not conform to specified child name")));

	if (!fcinfo->args[3].isnull)
	{
		var = PG_GETARG_XML_P(3);
		cnodestr = VARDATA(var);
		len = VARSIZE(var) - VARHDRSZ;
		nodestring = ivy_xmlCharStrndup(cnodestr, len);
	}
	else
		PG_RETURN_XML_P(data);

	ivy_xml_check_child_path(xpath_expr);

	initStringInfo(&result);
	appendStringInfoString(&result, xpath_expr);
	appendStringInfoString(&result, "/");
	appendStringInfoString(&result, c_expr);

	full_path = cstring_to_text(result.data);
	pfree(result.data);

	ws = ivy_xml_workspace();	/* BUG#Z9218 */

	res = ivy_xml_xpath(ws, full_path, data, NULL);
	ivy_xml_addnextsiblingnode(res, nodestring);
	ret = (xmltype *)ivy_xml_xmlnode2xmltype((xmlNodePtr)ws->doc);
	ret = (xmltype *)rv_newline((text *)ret);

	cleanup_ws(ws);

	PG_RETURN_XML_P(ret);
#else
	NO_XML_SUPPORT();
	return 0;
#endif
}

Datum
ivy_insertchildxmlafter2(PG_FUNCTION_ARGS)
{
#ifdef USE_LIBXML
	xmltype    *data = NULL;
	text	   *xpath_expr_text = NULL;
	text	   *child_expr_text = NULL;
	xmltype    *var = NULL;
	text	   *ns = NULL;
	xmltype    *ret = NULL;
	char	   *c_expr = NULL;
	char	   *xpath_expr = NULL;
	char	   *cns = NULL;
	text	   *full_path = NULL;
	char	   *cnodestr = NULL;
	int 		len;
	xpath_ws   *ws = NULL;
	xmlChar	   *nodestring = NULL;
	xmlXPathObjectPtr res;

	StringInfoData result;

	if (fcinfo->args[0].isnull)
		PG_RETURN_NULL();
	else
		data = PG_GETARG_XML_P(0);

	if (fcinfo->args[1].isnull)
		PG_RETURN_NULL();
	else
	{
		xpath_expr_text = PG_GETARG_TEXT_PP(1);
		xpath_expr = text_to_cstring(xpath_expr_text);
	}

	if (!fcinfo->args[2].isnull)
	{
		child_expr_text = PG_GETARG_TEXT_PP(2);
		c_expr = text_to_cstring(child_expr_text);
	}
	else
		ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
				errmsg("The document being inserted does not conform to specified child name")));

	if (!fcinfo->args[3].isnull)
	{
		var = PG_GETARG_XML_P(3);
		cnodestr = VARDATA(var);
		len = VARSIZE(var) - VARHDRSZ;
		nodestring = ivy_xmlCharStrndup(cnodestr, len);
	}
	else
		PG_RETURN_XML_P(data);

	if (!fcinfo->args[4].isnull)
	{
		ns = PG_GETARG_TEXT_PP(4);
		cns = text_to_cstring(ns);
	}

	ivy_xml_check_child_path(xpath_expr);

	initStringInfo(&result);
	appendStringInfoString(&result, xpath_expr);
	appendStringInfoString(&result, "/");
	appendStringInfoString(&result, c_expr);

	full_path = cstring_to_text(result.data);
	pfree(result.data);

	ws = ivy_xml_workspace();	/* BUG#Z9218 */

	res = ivy_xml_xpath(ws, full_path, data, cns);
	ivy_xml_addnextsiblingnode(res, nodestring);
	ret = (xmltype *)ivy_xml_xmlnode2xmltype((xmlNodePtr)ws->doc);
	ret = (xmltype *)rv_newline((text *)ret);

	cleanup_ws(ws);

	PG_RETURN_XML_P(ret);
#else
	NO_XML_SUPPORT();
	return 0;
#endif
}

xmltype *
updatexml(List *args)
{
#ifdef USE_LIBXML
	ListCell		*f;
	ListCell		*l;
	ListCell		*v;
	List			*tmp;
	List			*datap;
	List	   		*nsp;
	char			*ns = NULL;
	char			*cxmldata = NULL;
	xmltype			*xmldata = NULL;
	xmltype			*ret = NULL;
	int 			narg = 0;
	xpath_ws   *ws = NULL;
	bool			has_namespace = false;
	xmlXPathObjectPtr res;


	if (args == NIL)
		ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
				errmsg("invalid XPath expression")));

	/* get the xml data */
	f = list_head(args);
	if (f->ptr_value != NULL)
	{
		datap = lfirst(f);
		xmldata = (xmltype *)DatumGetTextP(PointerGetDatum(datap));
		cxmldata = text_to_cstring(DatumGetTextP(PointerGetDatum(datap)));
		if (strcmp(cxmldata, "") == 0)
			return xmldata;
	}
	else
		return NULL;

	ws = ivy_xml_workspace();	/* BUG#Z9218 */
	init_ws_doc(ws, xmldata);

	/* get the namespace_string if it exist */
	if (args->length % 2 == 0)
	{
		l = list_tail(args);
		nsp= lfirst(l);
		if (nsp != NULL)
		{
			ns = text_to_cstring(DatumGetTextP(PointerGetDatum(nsp)));
			has_namespace = true;
		}

		args = list_delete_last(args);
	}

	tmp = list_delete_first(args);
	if (tmp == NIL || tmp->length % 2 != 0)
		ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
				errmsg("invalid XPath expression")));

	v = list_head(tmp);
	for (;;)
	{
		char	   *varstr = NULL;
		char	   *cxpath = NULL;
		xmlChar	   *string = NULL;
		xmlDocPtr	vdoc = NULL;
		bool		value_is_document;
		text	   *xpath = NULL;
		text	   *var = NULL;
		int			len2 = 0;

		if (v == NULL)
			ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
					errmsg("invalid XPath expression")));

		/* get xpath_string */
		if (v->ptr_value != NULL)
		{
			xpath = DatumGetTextP(PointerGetDatum(lfirst(v)));
			cxpath = text_to_cstring(xpath);
			if (strcmp(cxpath, "") == 0)
				ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
						errmsg("invalid XPath expression")));
		}
		else /* handle the null */
		{
			ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
					errmsg("invalid XPath expression")));
		}

		v = lnext(tmp, v);
		if (v == NULL)
			ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
					errmsg("invalid XPath expression")));
		if (v->ptr_value != NULL)
		{
			/* get value_expr */
			var = DatumGetTextP(PointerGetDatum(lfirst(v)));

			varstr = VARDATA(var);
			len2 = VARSIZE(var) - VARHDRSZ;
			string = ivy_xmlCharStrndup(varstr, len2);
		}
		else
		{
			string = (xmlChar *)"";
		}

		v = lnext(tmp, v);

		if (has_namespace)
			res = ivy_xml_xpath2(ws, xpath, ws->doc, ns);
		else
			res = ivy_xml_xpath2(ws, xpath, ws->doc, NULL);
		/* Begin - BUG#Z9218 */
		vdoc = ivy_xml_parse_document((const char *) string, xmlStrlen(string), 0);
		value_is_document = vdoc != NULL;
		xmlFreeDoc(vdoc);

		/* update the node value */
		if (!value_is_document && strstr(cxpath, "text()") &&
			!(res->type == XPATH_NODESET && res->nodesetval != NULL &&
			  res->nodesetval->nodeNr > 0 &&
			  res->nodesetval->nodeTab[0]->type == XML_DOCUMENT_NODE))
		{
			ivy_xml_setnodecontent(res, string);
		}
		else /* replace a node */
		{
			bool replaced = ivy_xml_replacenode(ws, string);

			if (!replaced)
			{
				cleanup_ws(ws);
				return NULL;
			}
		}
		/* End - BUG#Z9218 */

		ret = (xmltype *)ivy_xml_xmlnode2xmltype((xmlNodePtr)ws->doc);
		ret = (xmltype *)rv_newline((text *)ret);

		narg += 2;
		if (narg == tmp->length)
			break;
	}

	cleanup_ws(ws);

	return ret;
#else
	NO_XML_SUPPORT();
	return NULL;
#endif
}
