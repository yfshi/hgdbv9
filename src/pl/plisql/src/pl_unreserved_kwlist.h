/*-------------------------------------------------------------------------
 *
 * pl_unreserved_kwlist.h
 *
 * The keyword lists are kept in their own source files for use by
 * automatic tools.  The exact representation of a keyword is determined
 * by the PG_KEYWORD macro, which is not defined in this file; it can
 * be defined by the caller for special purposes.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/pl/plisql/src/pl_unreserved_kwlist.h
 *
 * add the file for requirement "SQL PARSER"
 *
 *-------------------------------------------------------------------------
 */

/* There is deliberately not an #ifndef PL_UNRESERVED_KWLIST_H here. */

/*
 * List of (keyword-name, keyword-token-value) pairs.
 *
 * Be careful not to put the same word into pl_reserved_kwlist.h.  Also be
 * sure that pl_gram.y's unreserved_keyword production agrees with this list.
 *
 * Note: gen_keywordlist.pl requires the entries to appear in ASCII order.
 */

/* name, value */
PG_KEYWORD("absolute", K_ABSOLUTE)
/* Begin - SRS-PLSQL-SUBPROC */
PG_KEYWORD("accessible", K_ACCESSIBLE)
/* End - SRS-PLSQL-SUBPROC */
PG_KEYWORD("alias", K_ALIAS)
PG_KEYWORD("and", K_AND)
PG_KEYWORD("array", K_ARRAY)
/* Begin - SRS-PLSQL-SUBPROC */
PG_KEYWORD("as", K_AS)
/* End - SRS-PLSQL-SUBPROC */
PG_KEYWORD("assert", K_ASSERT)
/* Begin - SRS-PLSQL-SUBPROC */
PG_KEYWORD("authid", K_AUTHID)
/* End - SRS-PLSQL-SUBPROC */
PG_KEYWORD("backward", K_BACKWARD)
PG_KEYWORD("between", K_BETWEEN)	/* ReqID:SRS-PLSQL-FORALL */
/* Begin - ReqID:SRS-PLSQL-BULKCOLLECT */
PG_KEYWORD("bulk", K_BULK)
/* End - ReqID:SRS-PLSQL-BULKCOLLECTt */
PG_KEYWORD("call", K_CALL)
PG_KEYWORD("chain", K_CHAIN)
PG_KEYWORD("close", K_CLOSE)
PG_KEYWORD("collate", K_COLLATE)
/* Begin - SRS-PLSQL-SUBPROC */
PG_KEYWORD("collation", K_COLLATION)
/* End - SRS-PLSQL-SUBPROC */
/* Begin - ReqID:SRS-PLSQL-BULKCOLLECT */
PG_KEYWORD("collect", K_COLLECT)
/* End - ReqID:SRS-PLSQL-BULKCOLLECTt */
PG_KEYWORD("column", K_COLUMN)
PG_KEYWORD("column_name", K_COLUMN_NAME)
PG_KEYWORD("commit", K_COMMIT)
PG_KEYWORD("constant", K_CONSTANT)
PG_KEYWORD("constraint", K_CONSTRAINT)
PG_KEYWORD("constraint_name", K_CONSTRAINT_NAME)
PG_KEYWORD("continue", K_CONTINUE)
PG_KEYWORD("count", K_COUNT)	/* ReqID:SRS-PLSQL-Collection */
PG_KEYWORD("current", K_CURRENT)
/* Begin - SRS-PLSQL-SUBPROC */
PG_KEYWORD("current_user", K_CURRENT_USER)
/* End - SRS-PLSQL-SUBPROC */
PG_KEYWORD("cursor", K_CURSOR)
PG_KEYWORD("datatype", K_DATATYPE)
PG_KEYWORD("debug", K_DEBUG)
PG_KEYWORD("default", K_DEFAULT)
/* Begin - SRS-PLSQL-SUBPROC */
PG_KEYWORD("definer", K_DEFINER)
PG_KEYWORD("delete", K_DELETE)	/* ReqID:SRS-PLSQL-FORALL */
/* End - SRS-PLSQL-SUBPROC */
PG_KEYWORD("detail", K_DETAIL)
/* Begin - SRS-PLSQL-SUBPROC */
PG_KEYWORD("deterministic", K_DETERMINISTIC)
/* End - SRS-PLSQL-SUBPROC */
PG_KEYWORD("diagnostics", K_DIAGNOSTICS)
PG_KEYWORD("do", K_DO)
PG_KEYWORD("dump", K_DUMP)
PG_KEYWORD("elseif", K_ELSIF)
PG_KEYWORD("elsif", K_ELSIF)
PG_KEYWORD("errcode", K_ERRCODE)
PG_KEYWORD("error", K_ERROR)
PG_KEYWORD("exception", K_EXCEPTION)
/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
PG_KEYWORD("exception_init", K_EXCEPTION_INIT)
/* End - ReqID:SRS-PLSQL-EXCEPTION */
PG_KEYWORD("exists", K_EXISTS)	/* ReqID:SRS-PLSQL-Collection */
PG_KEYWORD("exit", K_EXIT)
PG_KEYWORD("extend", K_EXTEND)	/* ReqID:SRS-PLSQL-Collection */
PG_KEYWORD("fetch", K_FETCH)
PG_KEYWORD("first", K_FIRST)
PG_KEYWORD("forall", K_FORALL)	/* ReqID:SRS-PLSQL-FORALL */
PG_KEYWORD("forward", K_FORWARD)
PG_KEYWORD("get", K_GET)
PG_KEYWORD("goto", K_GOTO)      /* ReqID:SRS-PLSQL-GOTO */
PG_KEYWORD("hint", K_HINT)
PG_KEYWORD("immediate", K_IMMEDIATE)	/* BUG#Z1438 */
PG_KEYWORD("import", K_IMPORT)
PG_KEYWORD("index", K_INDEX)	/* ReqID:SRS-PLSQL-Collection */
PG_KEYWORD("indices", K_INDICES)	/* ReqID:SRS-PLSQL-FORALL */
PG_KEYWORD("info", K_INFO)
PG_KEYWORD("insert", K_INSERT)
PG_KEYWORD("is", K_IS)
PG_KEYWORD("last", K_LAST)
PG_KEYWORD("limit", K_LIMIT)	/* ReqID:SRS-PLSQL-Collection */
PG_KEYWORD("log", K_LOG)
PG_KEYWORD("merge", K_MERGE)	/* BUG#Z1389 */
PG_KEYWORD("message", K_MESSAGE)
PG_KEYWORD("message_text", K_MESSAGE_TEXT)
PG_KEYWORD("move", K_MOVE)
PG_KEYWORD("next", K_NEXT)
PG_KEYWORD("no", K_NO)
/* Begin - SRS-PLSQL-SUBPROC */
PG_KEYWORD("nocopy", K_NOCOPY)
/* End - SRS-PLSQL-SUBPROC */
PG_KEYWORD("notice", K_NOTICE)
PG_KEYWORD("of", K_OF)		/* ReqID:SRS-PLSQL-Collection */
PG_KEYWORD("open", K_OPEN)
PG_KEYWORD("option", K_OPTION)
/* Begin - SRS-PLSQL-SUBPROC */
PG_KEYWORD("out", K_OUT)
PG_KEYWORD("package", K_PACKAGE)
PG_KEYWORD("parallel_enable", K_PARALLEL_ENABLE)
/* End - SRS-PLSQL-SUBPROC*/
PG_KEYWORD("perform", K_PERFORM)
PG_KEYWORD("pg_context", K_PG_CONTEXT)
PG_KEYWORD("pg_datatype_name", K_PG_DATATYPE_NAME)
PG_KEYWORD("pg_exception_context", K_PG_EXCEPTION_CONTEXT)
PG_KEYWORD("pg_exception_detail", K_PG_EXCEPTION_DETAIL)
PG_KEYWORD("pg_exception_hint", K_PG_EXCEPTION_HINT)
/* Begin - SRS-PLSQL-SUBPROC */
PG_KEYWORD("pipelined", K_PIPELINED)
/* End - SRS-PLSQL-SUBPROC */
/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
PG_KEYWORD("pragma", K_PRAGMA)
/* End - ReqID:SRS-PLSQL-EXCEPTION */
PG_KEYWORD("print_strict_params", K_PRINT_STRICT_PARAMS)
PG_KEYWORD("prior", K_PRIOR)
PG_KEYWORD("query", K_QUERY)
PG_KEYWORD("raise", K_RAISE)
/* Begin - ReqID:SRS-PLSQL-SUBTYPE */
PG_KEYWORD("range", K_RANGE)
/* End - ReqID:SRS-PLSQL-SUBTYPE */
/* Begin - ReqID:SRS-PLSQL-RECORD */
PG_KEYWORD("record", K_RECORD)
/* End - ReqID:SRS-PLSQL-RECORD */
/* Begin - ReqID:SRS-PLSQL-CURSOR */
PG_KEYWORD("ref", K_REF)
/* End - ReqID:SRS-PLSQL-CURSOR */
PG_KEYWORD("relative", K_RELATIVE)
/* Begin - SRS-PLSQL-SUBPROC */
PG_KEYWORD("relies_on", K_RELIES_ON)
PG_KEYWORD("result_cache", K_RESULT_CACHE)
/* End - SRS-PLSQL-SUBPROC*/
PG_KEYWORD("return", K_RETURN)
PG_KEYWORD("returned_sqlstate", K_RETURNED_SQLSTATE)
/* Begin - ReqID:SRS-PLSQL-DSQL */
PG_KEYWORD("returning", K_RETURNING)
/* End - ReqID:SRS-PLSQL-DSQL */
PG_KEYWORD("reverse", K_REVERSE)
PG_KEYWORD("rollback", K_ROLLBACK)
PG_KEYWORD("row_count", K_ROW_COUNT)
PG_KEYWORD("rownum", K_ROWNUM)	/* BUG#M0000450 */
PG_KEYWORD("rowtype", K_ROWTYPE)
PG_KEYWORD("schema", K_SCHEMA)
PG_KEYWORD("schema_name", K_SCHEMA_NAME)
PG_KEYWORD("scroll", K_SCROLL)
/* Begin - ReqID:SRS-PLSQL-CURSOR */
PG_KEYWORD("select", K_SELECT)
/* End - ReqID:SRS-PLSQL-CURSOR */
PG_KEYWORD("slice", K_SLICE)
PG_KEYWORD("sqlstate", K_SQLSTATE)
PG_KEYWORD("stacked", K_STACKED)
/* Begin - ReqID:SRS-PLSQL-SUBTYPE */
PG_KEYWORD("subtype", K_SUBTYPE)
/* End - ReqID:SRS-PLSQL-SUBTYPE */
PG_KEYWORD("table", K_TABLE)
PG_KEYWORD("table_name", K_TABLE_NAME)
/* Begin - SRS-PLSQL-SUBPROC */
PG_KEYWORD("trigger", K_TRIGGER)
/* End - SRS-PLSQL-SUBPROC */
PG_KEYWORD("trim", K_TRIM)	/* ReqID:SRS-PLSQL-Collection */
PG_KEYWORD("type", K_TYPE)
PG_KEYWORD("update", K_UPDATE)	/* ReqID:SRS-PLSQL-FORALL */
PG_KEYWORD("use_column", K_USE_COLUMN)
PG_KEYWORD("use_variable", K_USE_VARIABLE)
/* Begin - SRS-PLSQL-SUBPROC*/
PG_KEYWORD("using_nls_comp", K_USING_NLS_COMP)
/* End - SRS-PLSQL-SUBPROC */
PG_KEYWORD("values", K_VALUES)	/* ReqID:SRS-PLSQL-FORALL */
PG_KEYWORD("variable_conflict", K_VARIABLE_CONFLICT)
/* Begin - ReqID:SRS-PLSQL-Collection */
PG_KEYWORD("varray", K_VARRAY)
PG_KEYWORD("varying", K_VARYING)
/* End - ReqID:SRS-PLSQL-Collection */
PG_KEYWORD("warning", K_WARNING)
