/*-------------------------------------------------------------------------
 *
 * mysshow.c
 *	  Implenmentation of the MySQL SHOW functionality
 *
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 *
 * IDENTIFICATION
 *	  contrib/ivorysql_my/src/show/mysshow.c
 *
 * add the file for requirement "SRS-MYSQL-SHOW"
 *
 *-------------------------------------------------------------------------
 */
#ifdef WIN32
#pragma execution_character_set("utf-8")
#endif
 
 #include "c.h"
#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "catalog/pg_type.h"
#include "commands/sequence.h"
#include "executor/execdesc.h"
#include "executor/executor.h"
#include "executor/tstoreReceiver.h"
#include "mb/pg_wchar.h"
#include "nodes/plannodes.h"
#include "nodes/nodes.h"
#include "nodes/nodeFuncs.h"
#include "parser/analyze.h"
#include "parser/parser.h"
#include "parser/parse_type.h"
#include "parser/scansup.h"
#include "rewrite/rewriteHandler.h"
#include "tcop/dest.h"
#include "tcop/tcopprot.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/numeric.h"
#include "utils/snapmgr.h"
#include "utils/timestamp.h"
#include "utils/varbit.h"
#include "nodes/makefuncs.h"
#include "utils/syscache.h"
#include "utils/datum.h"
#include "optimizer/clauses.h"
#include "mb/pg_wchar.h"
#include "parser/pg_object_hook.h"
#include "catalog/pg_namespace.h"
#include "utils/jsonfuncs.h"
#include "catalog/pg_class.h"
#include "access/table.h"
#include "utils/rel.h"
#include "lib/stringinfo.h"
#include "access/skey.h"
#include "catalog/index.h"	/* SRS-MYSQL-SQL-TableLEVELINDEX */
#include "catalog/pg_inherits.h"
#include "utils/fmgroids.h"
#include "access/genam.h"
#include "executor/spi.h"
#include "utils/lsyscache.h"
#include "utils/varlena.h" /* BUG#Z6281*/
#include "utils/ruleutils.h" /* BUG#Z6420*/

/* Begin - #Z6036 */

/***************************************************************************
 List all privileges supported
***************************************************************************/

typedef struct show_privileges_st
{
	const char *privilege;
	const char *context;
	const char *comment;
}show_privileges_st;

static struct show_privileges_st sys_privileges[] =
{
	{"Alter", "Tables",  "To alter the table"},
	{"Alter routine", "Functions,Procedures",  "To alter or drop stored functions/procedures"},
	{"Create", "Databases,Tables,Indexes",  "To create new databases and tables"},
	{"Create routine","Databases","To use CREATE FUNCTION/PROCEDURE"},
	{"Create temporary tables","Databases","To use CREATE TEMPORARY TABLE"},
	{"Create view", "Tables",  "To create new views"},
	{"Create user", "Server Admin",  "To create new users"},
	{"Delete", "Tables",  "To delete existing rows"},
	{"Drop", "Databases,Tables", "To drop databases, tables, and views"},
#ifndef EMBEDDED_LIBRARY
	{"Event","Server Admin","To create, alter, drop and execute events"},
#endif
	{"Execute", "Functions,Procedures", "To execute stored routines"},
	{"File", "File access on server",   "To read and write files on the server"},
	{"Grant option",  "Databases,Tables,Functions,Procedures", "To give to other users those privileges you possess"},
	{"Index", "Tables",  "To create or drop indexes"},
	{"Insert", "Tables",  "To insert data into tables"},
	{"Lock tables","Databases","To use LOCK TABLES (together with SELECT privilege)"},
	{"Process", "Server Admin", "To view the plain text of currently executing queries"},
	{"Proxy", "Server Admin", "To make proxy user possible"},
	{"References", "Databases,Tables", "To have references on tables"},
	{"Reload", "Server Admin", "To reload or refresh tables, logs and privileges"},
	{"Replication client","Server Admin","To ask where the slave or master servers are"},
	{"Replication slave","Server Admin","To read binary log events from the master"},
	{"Select", "Tables",  "To retrieve rows from table"},
	{"Show databases","Server Admin","To see all databases with SHOW DATABASES"},
	{"Show view","Tables","To see views with SHOW CREATE VIEW"},
	{"Shutdown","Server Admin", "To shut down the server"},
	{"Super","Server Admin","To use KILL thread, SET GLOBAL, CHANGE MASTER, etc."},
	{"Trigger","Tables", "To use triggers"},
	{"Create tablespace", "Server Admin", "To create/alter/drop tablespaces"},
	{"Update", "Tables",  "To update existing rows"},
	{"Usage","Server Admin","No privileges - allow connect only"},
	{NULL, NULL, NULL}
};

/* End - #Z6036 */


PG_FUNCTION_INFO_V1(mys_show_create_database);
PG_FUNCTION_INFO_V1(mys_show_create_table);
PG_FUNCTION_INFO_V1(mys_show_privileges);
PG_FUNCTION_INFO_V1(mys_show_warnings);
PG_FUNCTION_INFO_V1(show_next_autoinc);
PG_FUNCTION_INFO_V1(mysql_get_indexdef_columns); /* BUG#Z6420*/
/* Begin - SRS-MYSQL-SQL-TableLEVELINDEX */
PG_FUNCTION_INFO_V1(table_level_index_name);
PG_FUNCTION_INFO_V1(logical_index_keyname);
/* End - SRS-MYSQL-SQL-TableLEVELINDEX */

static void get_table_attribute(Oid tableoid, Form_pg_class pgclass, StringInfo attrbuf);
static void get_inherits(Oid tableoid, Form_pg_class pgclass, StringInfo inheritsbuf);
static text *ivy_JsonbValueAsText(JsonbValue *v);

/* Begin - SRS-MYSQL-SQL-TableLEVELINDEX */
/*
 * SQL-callable wrapper: return physical name from (index_name, table_name)
 */
Datum
table_level_index_name(PG_FUNCTION_ARGS)
{
	char	   *index_name = text_to_cstring(PG_GETARG_TEXT_PP(0));
	char	   *table_name = text_to_cstring(PG_GETARG_TEXT_PP(1));

	PG_RETURN_TEXT_P(cstring_to_text(construct_unique_index_name(index_name, table_name)));
}

/*
 * SQL-callable wrapper: return logical name for indexrelid
 */
Datum
logical_index_keyname(PG_FUNCTION_ARGS)
{
	Oid		indexrelid = PG_GETARG_OID(0);
	char	*name;
	name = GetLogicalIndexNameByIndex(indexrelid);
	if (name == NULL)
		PG_RETURN_NULL();
	PG_RETURN_TEXT_P(cstring_to_text(name));
}
/* End - SRS-MYSQL-SQL-TableLEVELINDEX */

/* Begin - #Z6036 */
/*
 * mys_show_privileges:
 *   Display the SQL statement information for privileges.
 */
Datum
mys_show_privileges(PG_FUNCTION_ARGS)
{
	#define		SHOW_PRIVILEGES_RESULTSET_NATTR 3
	FuncCallContext *funcctx;
	TupleDesc tupdesc;
	const int n_privs = sizeof(sys_privileges) / sizeof(sys_privileges[0]) - 1;

	if (SRF_IS_FIRSTCALL())
	{
		MemoryContext oldcontext;
		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		tupdesc = CreateTemplateTupleDesc(SHOW_PRIVILEGES_RESULTSET_NATTR);
		TupleDescInitEntry(tupdesc, 1, "Privilege", TEXTOID, -1, 0);
		TupleDescInitEntry(tupdesc, 2, "Context", TEXTOID, -1, 0);
		TupleDescInitEntry(tupdesc, 3, "Comment", TEXTOID, -1, 0);

		funcctx->tuple_desc = BlessTupleDesc(tupdesc);
		funcctx->max_calls = n_privs;
		funcctx->call_cntr = 0;

		MemoryContextSwitchTo(oldcontext);
	}

	funcctx = SRF_PERCALL_SETUP();

	if (funcctx->call_cntr < funcctx->max_calls)
	{

		Datum values[SHOW_PRIVILEGES_RESULTSET_NATTR];
		bool nulls[SHOW_PRIVILEGES_RESULTSET_NATTR] = {false, false, false};
		HeapTuple tuple;
		const show_privileges_st *row;

		row = &sys_privileges[funcctx->call_cntr];
		values[0] = CStringGetTextDatum(row->privilege);
		values[1] = CStringGetTextDatum(row->context);
		values[2] = CStringGetTextDatum(row->comment);

		tuple = heap_form_tuple(funcctx->tuple_desc, values, nulls);
		SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
	}

	SRF_RETURN_DONE(funcctx);
}

/*
 * mys_show_warnings:
 *   Display the SQL statement information for privileges.
 */
Datum
mys_show_warnings(PG_FUNCTION_ARGS)
{
	#define		SHOW_WARNINGS_RESULTSET_NATTR 3
	FuncCallContext *funcctx;
	TupleDesc tupdesc;

	if (SRF_IS_FIRSTCALL())
	{
		MemoryContext oldcontext;
		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		tupdesc = CreateTemplateTupleDesc(SHOW_WARNINGS_RESULTSET_NATTR);
		TupleDescInitEntry(tupdesc, 1, "Privilege", TEXTOID, -1, 0);
		TupleDescInitEntry(tupdesc, 2, "Context", TEXTOID, -1, 0);
		TupleDescInitEntry(tupdesc, 3, "Comment", TEXTOID, -1, 0);

		funcctx->tuple_desc = BlessTupleDesc(tupdesc);
		funcctx->max_calls = 0;
		funcctx->call_cntr = 0;

		MemoryContextSwitchTo(oldcontext);
	}

	funcctx = SRF_PERCALL_SETUP();

	SRF_RETURN_DONE(funcctx);
}
/* End - #Z6036 */

/*
 * ivy_JsonbValueAsText
 *
 * Return the text representation of the given JsonbValue.
 */
static text *
ivy_JsonbValueAsText(JsonbValue *v)
{
	switch (v->type)
	{
		case jbvNull:
			return NULL;

		case jbvBool:
			return v->val.boolean ?
				cstring_to_text_with_len("true", 4) :
				cstring_to_text_with_len("false", 5);

		case jbvString:
			return cstring_to_text_with_len(v->val.string.val,
											v->val.string.len);

		case jbvNumeric:
			{
				Datum		cstr;

				cstr = DirectFunctionCall1(numeric_out,
										   PointerGetDatum(v->val.numeric));

				return cstring_to_text(DatumGetCString(cstr));
			}

		case jbvBinary:
			{
				StringInfoData jtext;

				initStringInfo(&jtext);
				(void) JsonbToCString(&jtext, v->val.binary.data,
									  v->val.binary.len);

				return cstring_to_text_with_len(jtext.data, jtext.len);
			}

		default:
			elog(ERROR, "unrecognized jsonb type: %d", (int) v->type);
			return NULL;
	}
}

/*
 * mys_show_create_database:
 *   Display the SQL statement information for create the database.
 *
 * Parameters:
 *   Database name.
 *   exist flag.
 *
 * Returns:
 *   The string of create the database.
 */
Datum
mys_show_create_database(PG_FUNCTION_ARGS)
{
	Name				s = PG_GETARG_NAME(0);
	bool				existflg = PG_GETARG_BOOL(1);
	char			   *namestr;
	StringInfoData		buf;
	TupleDesc			tupdesc;
	MemoryContext		oldcontext;
	ReturnSetInfo	   *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	Tuplestorestate	   *tupstore;
	Datum				values[2];
	bool				nulls[2] = {false, false};
	HeapTuple			tuple;

	/* Prepare the column information to be returned */
	oldcontext = MemoryContextSwitchTo(rsinfo->econtext->ecxt_per_query_memory);
	tupdesc = CreateTemplateTupleDesc(2);
	TupleDescInitEntry(tupdesc, (AttrNumber) 1, "Database", TEXTOID, -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 2, "Create Database", TEXTOID, -1, 0);
	tupstore =
		tuplestore_begin_heap(rsinfo->allowedModes & SFRM_Materialize_Random,
							  false, work_mem);
	MemoryContextSwitchTo(oldcontext);

	/* Build the CREATE DATABASE statement */
	initStringInfo(&buf);
	namestr = pstrdup(NameStr(*s));
	appendStringInfoString(&buf, "CREATE DATABASE ");

	/* Used EXISTS in the SHOW statement */
	if (existflg)
	{
		appendStringInfoString(&buf, "/*!32312 IF NOT EXISTS*/ ");
	}

	/* if the GUC paramater sql_quote_show_create
	 * is on, database name need to add ``
	 * Begin - BUG#Z5947
	 * When the GUC parameter "sql_quote_show_create" is enabled,
	 * columns should also be enclosed in backticks.
	 */
	if (sql_quote_show_create)
	{
		if (has_sql_mode(MODE_ANSI_QUOTES))
		{
			appendStringInfo(&buf, " \"%s\" ", namestr);
		}
		else
		{
			appendStringInfo(&buf, " `%s` ", namestr);
		}
	}
	else
	{
		appendStringInfo(&buf, " %s ", namestr);
	}
	/* End - BUG#Z5947 */

	tuple = SearchSysCache1(NAMESPACENAME, PG_GETARG_DATUM(0));
	if (HeapTupleIsValid(tuple))
	{
		Form_pg_namespace	nspform;
		ObjectAddress		addr;
		PgObjectInfo		objinfo = NULL;
		char			   *server_encoding;

		/* If the database was created in mysql style,
		 * query the pg_object system catalog for its
		 * auxiliary information to help display the
		 * original information.
		 */
		server_encoding = TextDatumGetCString(DirectFunctionCall1(show_config_by_name,
											  CStringGetTextDatum("server_encoding")));
		nspform = (Form_pg_namespace) GETSTRUCT(tuple);
		ObjectAddressSet(addr, NamespaceRelationId, nspform->oid);
		if (pg_object_get_ori_infor_hook)
		{
			objinfo = pg_object_get_ori_infor_hook(addr);

			/* The original information ar create time is stored
			 * in the option column of the pg_object system catalog
			 * as key-value pairs under 'mysoption'.
			 */
			if (objinfo != NULL && objinfo->orioption != NULL)
			{
				text		   *key = DatumGetTextPP(cstring_to_text("mysoption"));
				JsonbValue	   *v;
				JsonbValue		vbuf;
				text		   *txt;

				v = getKeyJsonValueFromContainer(&objinfo->orioption->root,
												 VARDATA_ANY(key),
												 VARSIZE_ANY_EXHDR(key),
												 &vbuf);
				if (v != NULL && v->type != jbvNull)
				{
					txt = ivy_JsonbValueAsText(v);
					appendStringInfo(&buf, "/*!40100 DEFAULT %s */", text_to_cstring(txt));
				}
			}
			else
			{
				appendStringInfo(&buf, "/*!40100 DEFAULT CHARACTER SET %s */", server_encoding);
			}
		}
		else
		{
			appendStringInfo(&buf, "/*!40100 DEFAULT CHARACTER SET %s */", server_encoding);
		}

		ReleaseSysCache(tuple);
	}
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("Unknow database '%s'", namestr)));
	}

	/* Fill in the return values */
	values[0] = CStringGetTextDatum(namestr);
	values[1] = CStringGetTextDatum(pstrdup(buf.data));

	tuplestore_putvalues(tupstore, tupdesc, values, nulls);
	tuplestore_donestoring(tupstore);

	pfree(buf.data);
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupstore;
	rsinfo->setDesc = tupdesc;

	return (Datum) 0;
}

/*
 * get_table_attribute:
 *   Get the column attributes of the table.
 *
 * Parameters:
 *   tableoid: table oid
 *   pgclass: table informations
 *   attrbuf: return a string include all the column information finally.
 *
 * Returns:
 *   A string include all the column information finally.
 */
static void
get_table_attribute(Oid tableoid, Form_pg_class pgclass, StringInfo attrbuf)
{
	int					i;
	int 				actual_atts;
	StringInfoData		query_attribute_str;
	int					i_attname;
	int					i_attisdropped;
	int					i_attgenerated;
	int					i_attcollation;
	int					i_atttypid;
	int					i_attnotnull;
	int					i_attidentity;
	int					i_colcomment;
	int					i_attfdwoptions;
	int					i_coltype;
	int					i_coldefault;
	int					proc;
	char			   *attnamestr;
	char			   *attisdropped;
	char			   *attgeneratedstr;
	Oid					attcollationoid;
	Oid					atttypoid;
	char			   *attnotnullstr;
	char			   *attidentitystr;
	char			   *colcommentstr;
	char			   *coltypestr;
	char			   *coldefaultstr;
	char			   *attfdwoptions;
	StringInfoData		query_constraint_str;
	StringInfoData		constraint_resstr;
	int					i_constraintname;
	int					i_constraintdef;
	char			   *constraintname;
	char			   *constraintstr;
	SPITupleTable	   *spi_tuptable;
	TupleDesc			spi_tupdesc;

	actual_atts = 0;

	/* Query all columns of the table along with their default values */
	initStringInfo(&query_attribute_str);
	appendStringInfo(&query_attribute_str,
					 "select at.attname, at.attisdropped, at.attgenerated, at.attcollation,"
					 "at.atttypid, at.attnotnull, at.attidentity, replace(replace(attfdwoptions, '{', '('), '}', ')') as attfdwoptions,"
					 "pg_catalog.format_type(at.atttypid, at.atttypmod) as coltype,"
					 "pg_catalog.pg_get_expr(atdf.adbin, atdf.adrelid) as coldefault, "
					 "col_description(at.attrelid, at.attnum) as colcomment "
					 "from pg_attribute at "
					 "left join pg_attrdef atdf on (atdf.adrelid = at.attrelid and atdf.adnum = at.attnum)"
					 "where at.attrelid = %u and at.attnum > 0;",
					 pgclass->oid);

	if (SPI_connect() != SPI_OK_CONNECT)
		elog(ERROR, "SPI_connect failed");

	if (SPI_exec(query_attribute_str.data, 0) != SPI_OK_SELECT)
		elog(ERROR, "SPI_exec failed: %s", query_attribute_str.data);

	proc = SPI_processed;
	if (proc > 0)
	{
		spi_tuptable = SPI_tuptable;
		spi_tupdesc = spi_tuptable->tupdesc;

		for (i = 0; i < proc; i++)
		{
			HeapTuple	spi_tuple;
			bool		auto_increment =false;

			/* Get the position of the required filed */
			spi_tuple = spi_tuptable->vals[i];
			i_attname = SPI_fnumber(spi_tupdesc, "attname");
			i_attisdropped = SPI_fnumber(spi_tupdesc, "attisdropped");
			i_attgenerated = SPI_fnumber(spi_tupdesc, "attgenerated");
			i_attcollation = SPI_fnumber(spi_tupdesc, "attcollation");
			i_atttypid = SPI_fnumber(spi_tupdesc, "atttypid");
			i_coltype = SPI_fnumber(spi_tupdesc, "coltype");
			i_coldefault = SPI_fnumber(spi_tupdesc, "coldefault");
			i_attnotnull = SPI_fnumber(spi_tupdesc, "attnotnull");
			i_attidentity = SPI_fnumber(spi_tupdesc, "attidentity");
			i_colcomment = SPI_fnumber(spi_tupdesc, "colcomment");
			i_attfdwoptions = SPI_fnumber(spi_tupdesc, "attfdwoptions");

			/* Get the value of the required filed */
			attnamestr = SPI_getvalue(spi_tuple,spi_tupdesc, i_attname);
			attisdropped = SPI_getvalue(spi_tuple, spi_tupdesc, i_attisdropped);
			attgeneratedstr = SPI_getvalue(spi_tuple, spi_tupdesc, i_attgenerated);
			attcollationoid = atooid(SPI_getvalue(spi_tuple, spi_tupdesc, i_attcollation));
			atttypoid = atooid(SPI_getvalue(spi_tuple, spi_tupdesc, i_atttypid));
			coltypestr = SPI_getvalue(spi_tuple, spi_tupdesc, i_coltype);
			coldefaultstr = SPI_getvalue(spi_tuple, spi_tupdesc, i_coldefault);
			attnotnullstr = SPI_getvalue(spi_tuple, spi_tupdesc, i_attnotnull);
			attidentitystr = SPI_getvalue(spi_tuple, spi_tupdesc, i_attidentity);
			colcommentstr = SPI_getvalue(spi_tuple, spi_tupdesc, i_colcomment);
			attfdwoptions = SPI_getvalue(spi_tuple, spi_tupdesc, i_attfdwoptions);

			/* If the column is drop from the table,
			 * don't display the column information
			 */
			if (attisdropped != NULL && (strcmp(attisdropped, "t") == 0 ||
				strcmp(attisdropped, "1") == 0))	/* BUG#Z6017 */
			{
				continue;
			}

			/* Format properly if not first attr */
			if (actual_atts == 0)
			{
				appendStringInfoString(attrbuf, " (");
			}
			else
			{
				appendStringInfoString(attrbuf, ",");
			}
			appendStringInfoString(attrbuf, "\n    ");

			/* Attribute name */
			/* Begin - BUG#Z5947
			 * When the GUC parameter "sql_quote_show_create" is enabled,
			 * columns should also be enclosed in backticks.
			 */
			if (sql_quote_show_create)
			{
				if (has_sql_mode(MODE_ANSI_QUOTES))
				{
					appendStringInfo(attrbuf, "\"%s\"", attnamestr);
				}
				else
				{
					appendStringInfo(attrbuf, "`%s`", attnamestr);
				}
			}
			else
			{
				appendStringInfoString(attrbuf, attnamestr);
			}
			/* End - BUG#Z5947 */

			/* Attribute type */
			appendStringInfo(attrbuf, " %s", coltypestr);

			/* generated column */
			if (attgeneratedstr != NULL)
			{
				if (*attgeneratedstr == ATTRIBUTE_GENERATED_STORED)
				{
					appendStringInfo(attrbuf, " GENERATED ALWAYS AS (%s) STORED", attgeneratedstr);
				}
			}

			/* Information about the foreign table */
			if (attfdwoptions != NULL)
			{
				appendStringInfo(attrbuf, " OPTIONS %s", attfdwoptions);
			}

			/* Column is auto_increment */
			if (attidentitystr != NULL && attidentitystr[0] == ATTRIBUTE_IDENTITY_AUTO_INCREMENT)
			{
				auto_increment = true;
			}

			/* Column is NOT NULL */
			if (attnotnullstr != NULL && strcmp(attnotnullstr, "t") == 0)
			{
				appendStringInfoString(attrbuf, " NOT NULL");
			}
			else if (auto_increment)
			{
				/* Column is auto_increment need set NULL */
				appendStringInfoString(attrbuf, " NULL");
			}

			/* The defaule value for the column */
			if (coldefaultstr != NULL)
			{
				if (auto_increment)
				{
					appendStringInfoString(attrbuf, " AUTO_INCREMENT");
				}
				else if (strcmp(coldefaultstr, ""))
				{
					appendStringInfo(attrbuf, " DEFAULT %s", coldefaultstr);
				}
			}

			/* The collation for the column */
			if (OidIsValid(attcollationoid))
			{
				Oid			typcollation = get_typcollation(atttypoid);

				if (OidIsValid(typcollation) && attcollationoid != typcollation)
				{
					appendStringInfo(attrbuf, " COLLATE %s",
									 get_collation_name(attcollationoid));
				}
			}

			/* Column have comment */
			if (colcommentstr != NULL)
			{
				if (strcmp(colcommentstr, ""))
				{
					appendStringInfo(attrbuf, " COMMENT '%s'", colcommentstr);
				}
			}

			actual_atts++;
		}
	}

	if (SPI_finish() != SPI_OK_FINISH)
		elog(ERROR, "SPI_finish failed");

	initStringInfo(&query_constraint_str);
	initStringInfo(&constraint_resstr);

	/* Get all the constraint information */
	appendStringInfo(&query_constraint_str,
					 "select conname, pg_get_constraintdef(oid) as constraintdef from pg_constraint where conrelid = %u order by oid;",
					 pgclass->oid);

	if (SPI_connect() != SPI_OK_CONNECT)
		elog(ERROR, "SPI_connect failed");

	if (SPI_exec(query_constraint_str.data, 0) != SPI_OK_SELECT)
		elog(ERROR, "SPI_exec failed: %s", query_constraint_str.data);

	proc = SPI_processed;
	if (proc > 0)
	{
		spi_tuptable = SPI_tuptable;
		spi_tupdesc = spi_tuptable->tupdesc;

		/* Each constraint information is returned as a separate row */
		for (i = 0; i < proc; i++)
		{
			HeapTuple	spi_tuple;

			spi_tuple = spi_tuptable->vals[i];
			i_constraintname = SPI_fnumber(spi_tupdesc, "conname");
			i_constraintdef = SPI_fnumber(spi_tupdesc, "constraintdef");
			constraintname = pstrdup(SPI_getvalue(spi_tuple,spi_tupdesc, i_constraintname));
			constraintstr = pstrdup(SPI_getvalue(spi_tuple, spi_tupdesc, i_constraintdef));

			if (actual_atts == 0)
				appendStringInfoString(attrbuf, " (\n    ");
			else
				appendStringInfoString(attrbuf, ",\n    ");

			/* Begin - BUG#Z5947
			 * When the GUC parameter "sql_quote_show_create" is enabled,
			 * columns should also be enclosed in backticks.
			 */
			if (sql_quote_show_create)
			{
				if (has_sql_mode(MODE_ANSI_QUOTES))
				{
					appendStringInfo(attrbuf, "CONSTRAINT \"%s\" ", constraintname);
				}
				else
				{
					appendStringInfo(attrbuf, "CONSTRAINT `%s` ", constraintname);
				}
			}
			else
				appendStringInfo(attrbuf, "CONSTRAINT %s ", constraintname);
			/* End - BUG#Z5947 */

			appendStringInfoString(attrbuf, constraintstr);

			actual_atts++;
		}
	}

	if (SPI_finish() != SPI_OK_FINISH)
		elog(ERROR, "SPI_finish failed");

	/*Begin BUG#Z6281 BUG#Z6420 */
	/* Begin - SRS-MYSQL-SQL-TableLEVELINDEX */
	resetStringInfo(&query_attribute_str);
	appendStringInfo(&query_attribute_str,
					 "SELECT COALESCE("
					 "(SELECT opt.option_value "
					 " FROM pg_catalog.pg_options_to_table(ci.reloptions) "
					 " AS opt(option_name, option_value) "
					 " WHERE opt.option_name = 'logical_idxname' LIMIT 1), "
					 "ci.relname) AS \"INDEX_NAME\", "
					 "CASE WHEN ix.indisunique THEN 0 ELSE 1 END AS \"NON_UNIQUE\", "
					 "d.description AS \"INDEX_COMMENT\", "
					 "am.amname AS \"INDEX_TYPE\", "
					 "mys_information_schema.mysql_get_indexdef_columns(ix.indexrelid) "
					 "AS indexed_columns, "
					 "CASE WHEN ix.indexprs IS NOT NULL "
					 "THEN 1 ELSE 0 END::integer AS is_expr "
					 "FROM pg_catalog.pg_index ix "
					 "JOIN pg_catalog.pg_class ci ON ci.oid = ix.indexrelid "
					 "JOIN pg_catalog.pg_am am ON am.oid = ci.relam "
					 "LEFT JOIN pg_catalog.pg_description d ON d.objoid = ci.oid "
					 "WHERE ix.indrelid = %u "
					 "AND NOT ix.indisprimary "
					 "AND NOT EXISTS (SELECT 1 FROM pg_catalog.pg_constraint ct "
					 "WHERE ct.conindid = ix.indexrelid AND ct.contype = 'u') "
					 "ORDER BY 1, ix.indexrelid",
					 tableoid);
	/* End - SRS-MYSQL-SQL-TableLEVELINDEX */

	if (SPI_connect() != SPI_OK_CONNECT)
		elog(ERROR, "SPI_connect failed");

	if (SPI_exec(query_attribute_str.data, 0) != SPI_OK_SELECT)
		elog(ERROR, "SPI_exec failed: %s", query_attribute_str.data);

	proc = SPI_processed;
	if (proc > 0)
	{
		int i_indexname;
		int i_unique_flag;
		int i_indexcomment;
		int i_indexed_columns;
		int i_is_expr;
		int i_index_type;
		char *indexname = NULL;
		bool unique_flag = -1;
		char *indexcomment = NULL;
		char *indexed_columns = NULL;
		int is_expr = false;
		char *index_type = NULL;
		char *to_upper = NULL;

		spi_tuptable = SPI_tuptable;
		spi_tupdesc = spi_tuptable->tupdesc;
		for (i = 0; i < proc; i++)
		{
			HeapTuple	spi_tuple;
			indexcomment = NULL;
			index_type = NULL;

			spi_tuple = spi_tuptable->vals[i];
			i_indexname = SPI_fnumber(spi_tupdesc, "INDEX_NAME");
			i_unique_flag = SPI_fnumber(spi_tupdesc, "NON_UNIQUE");
			i_indexcomment = SPI_fnumber(spi_tupdesc, "INDEX_COMMENT");
			i_indexed_columns = SPI_fnumber(spi_tupdesc, "indexed_columns");
			i_is_expr = SPI_fnumber(spi_tupdesc, "is_expr");
			i_index_type = SPI_fnumber(spi_tupdesc, "INDEX_TYPE");

			indexname = pstrdup(SPI_getvalue(spi_tuple,spi_tupdesc, i_indexname));
			unique_flag = atoi(SPI_getvalue(spi_tuple,spi_tupdesc, i_unique_flag)) == 0? true:false;
			if (SPI_getvalue(spi_tuple,spi_tupdesc, i_indexcomment) != NULL)
				indexcomment = pstrdup(SPI_getvalue(spi_tuple,spi_tupdesc, i_indexcomment));
			indexed_columns = pstrdup(SPI_getvalue(spi_tuple, spi_tupdesc, i_indexed_columns));
			is_expr = atoi(SPI_getvalue(spi_tuple,spi_tupdesc, i_is_expr));
			index_type = pstrdup(SPI_getvalue(spi_tuple, spi_tupdesc, i_index_type));

			appendStringInfoString(attrbuf, ",");
			if (is_expr)
			{
				if (sql_quote_show_create)
				{
					if (has_sql_mode(MODE_ANSI_QUOTES))
					{
						if (unique_flag)
							appendStringInfo(attrbuf, "\n    UNIQUE KEY \"%s\" ", indexname);
						else
							appendStringInfo(attrbuf, "\n    KEY \"%s\" ", indexname);
					}
					else
					{
						if (unique_flag)
							appendStringInfo(attrbuf, "\n    UNIQUE KEY `%s` ", indexname);
						else
							appendStringInfo(attrbuf, "\n    KEY `%s` ", indexname);
					}
				}
				else
				{
					if (unique_flag)
						appendStringInfo(attrbuf, "\n    UNIQUE KEY %s ", indexname);
					else
						appendStringInfo(attrbuf, "\n    KEY %s ", indexname);
				}

				/* NOTE: this is temporary solution. when the future,
					"left"(col1::text, 10) will optimize*/
				appendStringInfo(attrbuf, "(%s)", indexed_columns);
				if (index_type)
				{
					to_upper = upcase_identifier(index_type, strlen(index_type), false, false);
					appendStringInfo(attrbuf, " USING %s", to_upper);
					pfree(to_upper);
				}

				if (indexcomment)
				{
					appendStringInfo(attrbuf, " COMMENT '%s'", indexcomment);
				}
			}
			else
			{
				if (sql_quote_show_create)
				{
					List *namelist = NIL;
					ListCell   *cell;
					int num = 0;

					SplitIdentifierString(indexed_columns, ',', &namelist);

					if (has_sql_mode(MODE_ANSI_QUOTES))
					{
						if (unique_flag)
							appendStringInfo(attrbuf, "\n    UNIQUE KEY \"%s\" (", indexname);
						else
							appendStringInfo(attrbuf, "\n    KEY \"%s\" (", indexname);
						foreach(cell, namelist)
						{
							if (num > 0)
								appendStringInfo(attrbuf, ",\"%s\"", (char *)lfirst(cell));
							else
								appendStringInfo(attrbuf, "\"%s\"", (char *)lfirst(cell));

							num++;
						}
					}
					else
					{
						if (unique_flag)
							appendStringInfo(attrbuf, "\n    UNIQUE KEY `%s` (", indexname);
						else
							appendStringInfo(attrbuf, "\n    KEY `%s` (", indexname);

						foreach(cell, namelist)
						{
							if (num > 0)
								appendStringInfo(attrbuf, ",`%s`", (char *)lfirst(cell));
							else
								appendStringInfo(attrbuf, "`%s`", (char *)lfirst(cell));

							num++;
						}
					}
					appendStringInfoString(attrbuf, ")");
				}
				else
				{
					if (unique_flag)
						appendStringInfo(attrbuf, "\n    UNIQUE KEY %s ", indexname);
					else
						appendStringInfo(attrbuf, "\n    KEY %s ", indexname);
					appendStringInfo(attrbuf, "(%s)", indexed_columns);
				}

				if (index_type)
				{
					to_upper = upcase_identifier(index_type, strlen(index_type), false, false);
					appendStringInfo(attrbuf, " USING %s", to_upper);
					pfree(to_upper);
				}

				if (indexcomment)
				{
					appendStringInfo(attrbuf, " COMMENT '%s'", indexcomment);
				}

			}
		}
	}

	if (SPI_finish() != SPI_OK_FINISH)
		elog(ERROR, "SPI_finish failed");
	/*End BUG#Z6281 Z6420*/

	/* Finish collecting the column information */
	if (actual_atts)
	{
		appendStringInfoString(attrbuf, "\n)");
	}
	else if (!OidIsValid(pgclass->reloftype))
	{
		appendStringInfoString(attrbuf, " (\n)");
	}

	/* free */
	pfree(query_attribute_str.data);
	pfree(query_constraint_str.data);
	pfree(constraint_resstr.data);
}

/*
 * get_inherits:
 *   Get the inherits of the table.
 *
 * Parameters:
 *   tableoid: table oid
 *   pgclass: table informations
 *   inheritsbuf: the table's inheritance relationship as a string.
 *
 * Returns:
 *   return the table's inheritance relationship as a string.
 */
static void
get_inherits(Oid tableoid, Form_pg_class pgclass, StringInfo inheritsbuf)
{
	Relation		inhrel;
	ScanKeyData		skey;
	SysScanDesc		inhscan;
	HeapTuple		inhtup;
	int				inhnum = 0;
	char			rekind = ' ';

	/* Open the system catalog pg_inherits and scan all parent tables
	 * of the specified table, but only consider ordinary as parents.
	 */
	inhrel = table_open(InheritsRelationId, AccessShareLock);
	ScanKeyInit(&skey,
				Anum_pg_inherits_inhrelid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(tableoid));

	inhscan = systable_beginscan(inhrel, InheritsRelidSeqnoIndexId, true,
								 NULL, 1, &skey);

	/* Process in a loop */
	while ((inhtup = systable_getnext(inhscan)) != NULL)
	{
		Form_pg_inherits inh = (Form_pg_inherits) GETSTRUCT(inhtup);
		Oid			inhparent = inh->inhparent;
		HeapTuple		parenttup;

		/* Get the parent table's kind */
		parenttup = SearchSysCache1(RELOID, ObjectIdGetDatum(inh->inhparent));
		if (HeapTupleIsValid(parenttup))
		{
			Form_pg_class	parentclass = NULL;

			parentclass = (Form_pg_class)GETSTRUCT(parenttup);
			rekind = parentclass->relkind;
			ReleaseSysCache(parenttup);
		}

		/* Use INHERITS if it is an oridinary table */
		if (rekind == RELKIND_RELATION)
		{
			if (inhnum == 0)
			{
				appendStringInfoString(inheritsbuf, "\nINHERITS (");
			}
			else
			{
				appendStringInfoString(inheritsbuf, ", ");
			}
			appendStringInfo(inheritsbuf, "%s ", get_rel_name(inhparent));
		}

		inhnum++;
	}

	if (inhnum > 0 && rekind == RELKIND_RELATION)
	{
		appendStringInfoString(inheritsbuf, ")");
	}

	systable_endscan(inhscan);

	/* clean up ... */
	table_close(inhrel, AccessShareLock);
}

/*
 * mys_show_create_table:
 *   Display the SQL statement information for create the table.
 *
 * Parameters:
 *   table oid.
 *
 * Returns:
 *   The string of create the table.
 */
Datum
mys_show_create_table(PG_FUNCTION_ARGS)
{
	Oid				reloid = PG_GETARG_OID(0);
	HeapTuple		reltup;
	Form_pg_class	pgclass = NULL;
	char		   *reltypename = NULL;
	char		   *relpersistence = "";
	char		   *qualrelname = NULL;
	char		   *reloftypename = NULL;
	StringInfoData		relcreate;
	StringInfoData		attrbuf;
	StringInfoData		inheritsbuf;
	StringInfoData		srvname;
	StringInfoData		ftoptions;
	StringInfoData		reloptiondata;
	char		   *reloptionsstr = NULL;
	char		   *partkeydef = "";
	char		   *srvnamestr = NULL;
	char		   *ftoptionsstr = NULL;
	char		   *formatstr = NULL;

	initStringInfo(&srvname);
	initStringInfo(&ftoptions);
	initStringInfo(&reloptiondata);

	/* Check if the table exists, and if it does,
	 * convert it to pg_class structure to retreive
	 * the required values.
	 */
	reltup = SearchSysCache1(RELOID, ObjectIdGetDatum(reloid));
	if (!HeapTupleIsValid(reltup))
	{
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("Table %u is not exists.", reloid)));
	}

	pgclass = (Form_pg_class)GETSTRUCT(reltup);

	/* Determin whether the table is an UNLOGGED table */
	if (pgclass->relpersistence == RELPERSISTENCE_UNLOGGED)
	{
		relpersistence = "UNLOGGED ";
	}

	if (pgclass->relpersistence == RELPERSISTENCE_TEMP)
	{
		/* Determin whether the table is an TEMPORARY table */
		reltypename = "TEMPORARY TABLE";
	}
	else if (pgclass->relkind == RELKIND_PARTITIONED_TABLE)
	{
		Datum		partdatum;

		/* Partitioned tables are also ordinary tables,
		 * need to retrieve their partition information.
		 */
		reltypename = "TABLE";

		partdatum = DirectFunctionCall1(pg_get_partkeydef, ObjectIdGetDatum(reloid));
		if (partdatum != 0)
			partkeydef = TextDatumGetCString(partdatum);
	}
	else if (pgclass->relkind == RELKIND_FOREIGN_TABLE)
	{
		StringInfoData	querybuf;
		int			i_srvname;
		int			i_ftoptions;

		/* foreign tables are also ordinary tables,
		 * need to retrieve their partition information.
		 */
		reltypename = "FOREIGN TABLE";

		/* retrieve name of foreign server and generic options */
		initStringInfo(&querybuf);
		appendStringInfo(&querybuf,
						  "SELECT fs.srvname, "
						  "pg_catalog.array_to_string(ARRAY("
						  "SELECT pg_catalog.quote_ident(option_name) || "
						  "' ' || pg_catalog.quote_literal(option_value) "
						  "FROM pg_catalog.pg_options_to_table(ftoptions) "
						  "ORDER BY option_name"
						  "), E',\n    ') AS ftoptions "
						  "FROM pg_catalog.pg_foreign_table ft "
						  "JOIN pg_catalog.pg_foreign_server fs "
						  "ON (fs.oid = ft.ftserver) "
						  "WHERE ft.ftrelid = '%u'",
						  reloid);

		if (SPI_connect() != SPI_OK_CONNECT)
			elog(ERROR, "SPI_connect failed");

		if (SPI_exec(querybuf.data, 0) != SPI_OK_SELECT)
			elog(ERROR, "SPI_exec failed: %s", querybuf.data);

		/* Determin the position of generated columns in the query result */
		i_srvname = SPI_fnumber(SPI_tuptable->tupdesc, "srvname");
		i_ftoptions = SPI_fnumber(SPI_tuptable->tupdesc, "ftoptions");

		/* get these columns values from the results */
		srvnamestr = pstrdup(SPI_getvalue(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, i_srvname));
		ftoptionsstr = pstrdup(SPI_getvalue(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, i_ftoptions));

		appendStringInfoString(&srvname, srvnamestr);
		appendStringInfoString(&ftoptions, ftoptionsstr);

		if (SPI_finish() != SPI_OK_FINISH)
			elog(ERROR, "SPI_finish failed");

		pfree(querybuf.data);
	}
	else
	{
		/* Otherwith, the relation type is table */
		reltypename = "TABLE";
	}

	/* table name */
	qualrelname = NameStr(pgclass->relname);
	if (pgclass->reloftype != 0)
	{
		reloftypename = get_typename(pgclass->reloftype);
	}

	/* Begin - BUG#Z5947
	 * When the GUC parameter "sql_quote_show_create" is enabled,
	 * columns should also be enclosed in backticks.
	 */
	if (sql_quote_show_create)
	{
		if (has_sql_mode(MODE_ANSI_QUOTES))
		{
			formatstr = "CREATE %s%s \"%s\"";
		}
		else
		{
			formatstr = "CREATE %s%s `%s`";
		}
	}
	else
	{
		formatstr = "CREATE %s%s %s";
	}
	/* End - BUG#Z5947 */

	/* Start assembling the CREATE TABLE statement */
	initStringInfo(&relcreate);
	appendStringInfo(&relcreate, formatstr,
					 relpersistence,
					 reltypename,
					 qualrelname);

	if (reloftypename != NULL)
		appendStringInfo(&relcreate, " OF %s", reloftypename);
	else
	{
		StringInfoData	querybuf;
		int			i_reloptions;

		/* Retrieve the table's reloptions informations */
		initStringInfo(&querybuf);
		appendStringInfo(&querybuf,
						  "SELECT array_remove(array_remove(c.reloptions,'check_option=local'),'check_option=cascaded') AS reloptions "
						  "FROM pg_class c where c.oid = %u",
						  reloid);

		if (SPI_connect() != SPI_OK_CONNECT)
			elog(ERROR, "SPI_connect failed");

		if (SPI_exec(querybuf.data, 0) != SPI_OK_SELECT)
			elog(ERROR, "SPI_exec failed: %s", querybuf.data);

		if (SPI_processed > 0)
		{
			/* Retrieve the position and value. */
			i_reloptions = SPI_fnumber(SPI_tuptable->tupdesc, "reloptions");
			reloptionsstr = SPI_getvalue(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, i_reloptions);

			/* Save the values to prevent them disappearing after SPI ends. */
			if (reloptionsstr != NULL)
			{
				appendStringInfoString(&reloptiondata, reloptionsstr);
			}
		}
		if (SPI_finish() != SPI_OK_FINISH)
			elog(ERROR, "SPI_finish failed");

		pfree(querybuf.data);

		/* Retrieve the column information of the table */
		initStringInfo(&attrbuf);
		get_table_attribute(reloid, pgclass ,&attrbuf);
		appendStringInfoString(&relcreate, attrbuf.data);

		pfree(attrbuf.data);

		/* Retrieve the inherits information of the table */
		initStringInfo(&inheritsbuf);
		get_inherits(reloid, pgclass ,&inheritsbuf);
		appendStringInfoString(&relcreate, inheritsbuf.data);

		pfree(inheritsbuf.data);

		/* Paritition table's partition information */
		if (pgclass->relkind == RELKIND_PARTITIONED_TABLE)
			appendStringInfo(&relcreate, "\nPARTITION BY %s", partkeydef);
		else if (pgclass->relkind == RELKIND_FOREIGN_TABLE)
			appendStringInfo(&relcreate, "\nSERVER %s", srvname.data);

		/* General WITH information of the table */
		if (reloptionsstr != NULL)
		{
			appendStringInfoString(&relcreate, "\nWITH (");
			if (reloptiondata.data != NULL)
			{
				appendStringInfo(&relcreate, "%s", reloptiondata.data);
			}
			appendStringInfoString(&relcreate, ")");
		}

		/* General OPTION information of the foreign table */
		if (ftoptions.data != NULL && strcmp(ftoptions.data, ""))
			appendStringInfo(&relcreate, "\nOPTIONS (\n    %s\n)", ftoptions.data);
	}

	/* Option information of the table ：
	 * engine, charset, comment, auto_increment
	 */
	{
		StringInfoData querybuf;
		int i_table_comment;
		uint64 auto_inc_num;
#ifdef NOT_USED
		int i_encoding;
		int i_collate;
		char *encodingstr = NULL;
		char *collatestr = NULL;
#endif

		char *table_commentstr = NULL;

		/* foreign tables are also ordinary tables,
		 * need to retrieve their partition information.
		 */
		reltypename = "FOREIGN TABLE";

		/* retrieve name of foreign server and generic options */
		initStringInfo(&querybuf);
		appendStringInfo(&querybuf,
						 "select obj_description('%u') as table_comment, "
						 "pg_encoding_to_char(encoding) as encoding, "
						 "replace(datcollate, '-', '') as collate "
						 "from pg_database "
						 "where datname = current_database()",
						 reloid);

		if (SPI_connect() != SPI_OK_CONNECT)
			elog(ERROR, "SPI_connect failed");

		if (SPI_exec(querybuf.data, 0) != SPI_OK_SELECT)
			elog(ERROR, "SPI_exec failed: %s", querybuf.data);

		/* Determin the position of generated columns in the query result */
		i_table_comment = SPI_fnumber(SPI_tuptable->tupdesc, "table_comment");
#ifdef NOT_USED
		i_encoding = SPI_fnumber(SPI_tuptable->tupdesc, "encoding");
		i_collate = SPI_fnumber(SPI_tuptable->tupdesc, "collate");
#endif

		/* get these columns values from the results */
		table_commentstr = SPI_getvalue(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, i_table_comment);
#ifdef NOT_USED
		encodingstr = SPI_getvalue(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, i_encoding);
		collatestr = SPI_getvalue(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, i_collate);
#endif

#ifdef NOT_USED
		appendStringInfoString(&relcreate, " ENGINE=InnoDB");
#endif
		auto_inc_num = get_show_autoinc(reloid);
		if (auto_inc_num > 1)
			appendStringInfo(&relcreate, " AUTO_INCREMENT=%lu", auto_inc_num);
#ifdef NOT_USED
		/* character and collate information are not diplayed,the interface is reserved
		   and the code will be submitted later.
		if (encodingstr)
			appendStringInfo(&relcreate, " DEFAULT CHARSET=%s", encodingstr);
		if (collatestr)
			appendStringInfo(&relcreate, " COLLATE=%s", collatestr);*/
#endif
		if (table_commentstr)
			appendStringInfo(&relcreate, " COMMENT='%s'", table_commentstr);

		if (SPI_finish() != SPI_OK_FINISH)
			elog(ERROR, "SPI_finish failed");

		pfree(querybuf.data);
	}

	ReleaseSysCache(reltup);

	/* free */
	pfree(srvname.data);
	pfree(ftoptions.data);
	pfree(reloptiondata.data);

	PG_RETURN_TEXT_P(cstring_to_text(relcreate.data));
}

/* Begin - BUG#Z6191 */
/*
 * get table next auto increment value by table oid
 * only get mysql's auto_increment type
 */
Datum
show_next_autoinc(PG_FUNCTION_ARGS)
{
	Oid reloid = PG_GETARG_OID(0);
	uint64 result = get_show_autoinc(reloid);

	PG_RETURN_UINT64(result);
}

/* Begin - BUG#Z6420 */
Datum
mysql_get_indexdef_columns(PG_FUNCTION_ARGS)
{
	Oid			indexrelid = PG_GETARG_OID(0);
	char	   *res = NULL;

	res = pg_get_indexdef_columns(indexrelid, true);

	if (res == NULL)
		PG_RETURN_NULL();

	PG_RETURN_TEXT_P(cstring_to_text(res));
}
/* End - BUG#Z6420 */
