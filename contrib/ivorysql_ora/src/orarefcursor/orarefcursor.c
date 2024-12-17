/*-------------------------------------------------------------------------
 *
 * File:orarefcursor.c
 *
 * Abstract:
 *		Definitions for the PLISQL cursor
 *
 * ReqID:SRS-PLSQL-CURSOR
 *
 * Authored by dwdai@highgo.com,20230315.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  contrib/ivorysql_ora/src/orarefcursor/orarefcursor.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include <ctype.h>
#include <limits.h>
#include "parser/scansup.h"
#include "utils/guc.h"
#include "../include/orarefcursor.h"
#include "utils/builtins.h"
#include "utils/ora_compatible.h"
#include "libpq/pqformat.h"
#include "funcapi.h"

typedef struct varlena orarefcursor;

typedef struct orarefcursorvalue
{
	void *type;
	bool isnull;
	char		values[FLEXIBLE_ARRAY_MEMBER];
} orarefcursorvalue;

#define OraRefCursorvalueSize(n)	(offsetof(orarefcursorvalue, values) + n + 1)
#define DatumGetOraRefCursor(d) (orarefcursor *) DatumGetPointer(d)

static orarefcursorvalue *DatumGetOraRefcursorValue(Datum datum);

PG_FUNCTION_INFO_V1(orarefcursorin);
/*
 *	orarefcursorin			- converts "..." to internal representation
 */
Datum
orarefcursorin(PG_FUNCTION_ARGS)
{
	char	   *inputText = PG_GETARG_CSTRING(0);
	orarefcursor *value;
	int total_size;
	int values_size;
	orarefcursorvalue *cursorvalue;

	values_size = strlen(inputText);
	total_size = OraRefCursorvalueSize(values_size);

	value = (orarefcursor *) palloc0(total_size + VARHDRSZ);
	SET_VARSIZE(value, total_size + VARHDRSZ);

	cursorvalue = (orarefcursorvalue *) VARDATA_ANY(value);

	cursorvalue->isnull = false;
	cursorvalue->type = NULL;
	memcpy(cursorvalue->values, inputText, values_size);
	return (Datum) value;
}

PG_FUNCTION_INFO_V1(orarefcursorout);
/*
 *	orarefcursorout			- converts internal representation to "..."
 */
Datum
orarefcursorout(PG_FUNCTION_ARGS)
{
	char *result;
	struct varlena *t = (struct varlena *) PG_GETARG_DATUM(0);
	orarefcursor *oracur = pg_detoast_datum_packed(t);
	int			len = VARSIZE_ANY_EXHDR(oracur);
	orarefcursorvalue *value;

	/* orarefcursorvalue may be compressed, we handle it like text_out */
	value = (orarefcursorvalue *) palloc0(len + 1);
	memcpy(value, VARDATA_ANY(oracur), len);
	if (value->isnull)
	{
		elog(ERROR, "orarefcursor is null");
	}
	else
	{
		if (identifier_case_switch == INTERCHANGE &&
			enable_case_switch)	/* case sensitive indentify */
			result = identifier_case_transform(value->values, strlen(value->values));
		else
			result = pstrdup(value->values);
	}
	if (t != oracur)
		pfree(oracur);
	pfree(value);
	PG_RETURN_CSTRING(result);
}


PG_FUNCTION_INFO_V1(orarefcursorrecv);
/*
 *	orarefcursorrecv - converts external binary format to PLSQL cursor_value
 */
Datum
orarefcursorrecv(PG_FUNCTION_ARGS)
{
	StringInfo	buf = (StringInfo) PG_GETARG_POINTER(0);
	int			nbytes;
	orarefcursor *cursorvar;
	orarefcursorvalue *cursor_value;
	char	*str;
	int str_bytes;

	str = pq_getmsgtext(buf, buf->len - buf->cursor, &str_bytes);

	nbytes = OraRefCursorvalueSize(str_bytes);
	cursorvar = (orarefcursor *) palloc0(nbytes + VARHDRSZ);

	SET_VARSIZE(cursorvar, nbytes + VARHDRSZ);
	cursor_value = (orarefcursorvalue *) VARDATA_ANY(cursorvar);

	cursor_value->isnull = false;
	cursor_value->type = NULL;
	memcpy(cursor_value->values, str, str_bytes);
	pfree(str);

	return (Datum) cursorvar;
}


PG_FUNCTION_INFO_V1(orarefcursorsend);
/*
 *	orarefcursorsend			- converts orarefcursor to binary format
 */
Datum
orarefcursorsend(PG_FUNCTION_ARGS)
{
	struct varlena *vl;
	orarefcursor *oracur;
	orarefcursorvalue *value;
	StringInfoData buf;
	text	   *t;
	int	len;

	/* orarefcursorvalue may be compressed, we handle it like text_out */
	vl = (struct varlena *) PG_GETARG_DATUM(0);
	oracur = pg_detoast_datum_packed(vl);
	len = VARSIZE_ANY_EXHDR(oracur);
	value = (orarefcursorvalue *) palloc0(len + 1);
	memcpy(value, VARDATA_ANY(oracur), len);
	if (value->isnull)
	{
		elog(ERROR, "orarefcursor is null");
	}
	else
	{
		if (identifier_case_switch == INTERCHANGE &&
			enable_case_switch)	/* case sensitive indentify */
			t = cstring_to_text(identifier_case_transform(value->values, strlen(value->values)));
		else
			t = cstring_to_text(value->values);
	}
	if (oracur != vl)
		pfree(oracur);
	pfree(value);

	pq_begintypsend(&buf);
	pq_sendtext(&buf, VARDATA_ANY(t), VARSIZE_ANY_EXHDR(t));
	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}


PG_FUNCTION_INFO_V1(sysrefcursorin);
/*
 * sys_refcursor in like textin
 */
Datum
sysrefcursorin(PG_FUNCTION_ARGS)
{
	char	   *inputText = PG_GETARG_CSTRING(0);

	PG_RETURN_TEXT_P(cstring_to_text(inputText));
}


PG_FUNCTION_INFO_V1(sysrefcursorout);

/*
 * sys_refcursor out like textout
 */
Datum
sysrefcursorout(PG_FUNCTION_ARGS)
{
	Datum		txt = PG_GETARG_DATUM(0);
	char *portalname;
	char *result;

	portalname = TextDatumGetCString(txt);

	if (identifier_case_switch == INTERCHANGE &&
		enable_case_switch)	/* case sensitive indentify */
		result = identifier_case_transform(portalname, strlen(portalname));
	else
		result = pstrdup(portalname);
	pfree(portalname);
	PG_RETURN_CSTRING(result);
}

/*
 * sys_refcursor send
 */
PG_FUNCTION_INFO_V1(sysrefcursorrecv);

Datum
sysrefcursorrecv(PG_FUNCTION_ARGS)
{
	StringInfo	buf = (StringInfo) PG_GETARG_POINTER(0);
	text	   *result;
	char	   *str;
	int			nbytes;

	str = pq_getmsgtext(buf, buf->len - buf->cursor, &nbytes);

	result = cstring_to_text_with_len(str, nbytes);
	pfree(str);
	PG_RETURN_TEXT_P(result);
}


/*
 * sys_refcursor send
 */
PG_FUNCTION_INFO_V1(sysrefcursorsend);

Datum
sysrefcursorsend(PG_FUNCTION_ARGS)
{
	text	   *t = PG_GETARG_TEXT_PP(0);
	StringInfoData buf;
	text		*new_t;

	if (identifier_case_switch == INTERCHANGE &&
		enable_case_switch)
	{
		char *portalname = TextDatumGetCString(t);

		new_t = cstring_to_text(identifier_case_transform(portalname,
					strlen(portalname)));
		pfree(portalname);
	}
	else
		new_t = t;

	pq_begintypsend(&buf);
	pq_sendtext(&buf, VARDATA_ANY(new_t), VARSIZE_ANY_EXHDR(new_t));
	if (new_t != t)
		pfree(new_t);
	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}



PG_FUNCTION_INFO_V1(orarefcursor_init);
/*
 * init orarefcursor function struct which will be used
 * in plisql
 */
Datum
orarefcursor_init(PG_FUNCTION_ARGS)
{
	orarefcursor_init_internal();
	PG_RETURN_VOID();
}


PG_FUNCTION_INFO_V1(syscursor_oracursor);
/*
 * convert sys_refcursor to orarefcursor
 */
Datum
syscursor_oracursor(PG_FUNCTION_ARGS)
{
	Datum		txt = PG_GETARG_DATUM(0);
	char		*portal;
	Datum		oraref;

	portal = TextDatumGetCString(txt);
	oraref = init_orarefcursor_value(NULL, portal);
	pfree(portal);

	return oraref;
}


PG_FUNCTION_INFO_V1(oracursor_syscursor);
/*
 * convert orarefcursor to sys_refcursor
 */
Datum
oracursor_syscursor(PG_FUNCTION_ARGS)
{
	Datum		oraref = PG_GETARG_DATUM(0);
	char		*portal;
	text		*sys_refcursor = NULL;

	portal = DatumGetOraRefCursorPortal(oraref);

	/* Begin - BUG#Z0001311 */
	/*
	 * return null should use PG_RETURN_NULL
	 */
	if (portal != NULL)
	{
		sys_refcursor = cstring_to_text(portal);
		pfree(portal);
		PG_RETURN_TEXT_P(sys_refcursor);
	}
	else
		PG_RETURN_NULL();
	/* End - BUG#Z00001311 */
}

/*
 * orarefcursor_init_internal init orarefcursor_funcs
 */
void
orarefcursor_init_internal(void)
{
	orarefcursor_funcs.OraRefcursorIsnull = OraRefcursorIsnull;
	orarefcursor_funcs.DatumGetOraRefCursorType = DatumGetOraRefCursorType;
	orarefcursor_funcs.init_orarefcursor_value = init_orarefcursor_value;
	orarefcursor_funcs.DatumGetOraRefCursorPortal = DatumGetOraRefCursorPortal;
	orarefcursor_funcs.OraRefcursorSetType = OraRefcursorSetType;

	orarefcursor_funcs.loadstate = LOADED;
}


/*
 * like text_to_cstring
 */
static orarefcursorvalue*
DatumGetOraRefcursorValue(Datum datum)
{
	struct varlena *t = (struct varlena *) datum;
	orarefcursor *syscur = pg_detoast_datum_packed(t);
	int			len = VARSIZE_ANY_EXHDR(syscur);
	orarefcursorvalue *value;

	/* orarefcursorvalue may be compressed, we handle it like text_out */
	value = (orarefcursorvalue *) palloc0(len + 1);
	memcpy(value, VARDATA_ANY(syscur), len);
	if (t != syscur)
		pfree(syscur);
	return value;
}


/*
 * get portal name from orarefcursor
 */
char*
DatumGetOraRefCursorPortal(Datum datum)
{
	char *portal;
	orarefcursorvalue *value;

	value = DatumGetOraRefcursorValue(datum);
	if (!value->isnull)
		portal = pstrdup(value->values);
	else
		portal = NULL;
	pfree(value);

	return portal;
}

/*
 * orarefcursor is null ?
 */
bool
OraRefcursorIsnull(Datum datum)
{
	bool isnull;
	orarefcursorvalue *value;

	value = DatumGetOraRefcursorValue(datum);
	isnull = value->isnull;
	pfree(value);

	return isnull;
}


/*
 * return orarefcursor type
 */
void*
DatumGetOraRefCursorType(Datum datum)
{
	void *rowtype;
	orarefcursorvalue *value;

	value = DatumGetOraRefcursorValue(datum);
	rowtype = value->type;
	pfree(value);

	return rowtype;
}


/*
 * init a orarefcursor with its type
 */
Datum
init_orarefcursor_value(void *type, const char *name)
{
	orarefcursor *new;
	int total_size;
	int value_size;
	orarefcursorvalue *nvalue;

	if (name == NULL)
		value_size = 0;
	else
		value_size = strlen(name);

	total_size = OraRefCursorvalueSize(value_size);

	new = (orarefcursor *) palloc0(total_size + VARHDRSZ);
	SET_VARSIZE(new, total_size + VARHDRSZ);

	nvalue = (orarefcursorvalue *) VARDATA_ANY(new);
	nvalue->type = type;
	nvalue->isnull = true;
	if (name != NULL)
	{
		memcpy(nvalue->values, name, value_size);
		nvalue->isnull = false;
	}

	return (Datum) new;
}


/*
 * set cursorvalue type address
 */
void
OraRefcursorSetType(void *type, Datum datum)
{
	struct varlena *t = (struct varlena *) datum;
	orarefcursor *syscur;
	orarefcursorvalue *value;

	if (datum == (Datum) 0)
		return;

	syscur = pg_detoast_datum_packed(t);

	if (t != syscur)
		elog(ERROR, "sys_refcursor has change memory");

	value = (orarefcursorvalue *) VARDATA_ANY(syscur);
	value->type = type;

	return;
}

