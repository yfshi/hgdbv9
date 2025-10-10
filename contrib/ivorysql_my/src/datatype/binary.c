/*-------------------------------------------------------------------------
 *
 * binary.c
 *   for mysql binary type gin index
 *
 * Portions Copyright (c) 2025 - , Highgo MySQL Compatible Development Group
 *
 * IDENTIFICATION
 *   contrib/ivorysql_my/src/datatype/binary/binary.c
 *
 * add the file for requirement "SRS-MYSQL-BINARYTYPE"
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "anybinary.h"
#include "catalog/pg_collation.h"
#include "catalog/pg_type.h"
#include "fmgr.h"
#include "lib/stringinfo.h"
#include "mb/pg_wchar.h"
#include "miscadmin.h"
#include "parser/parse_oper.h"
#include "strtod.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/elog.h"
#include "utils/fmgrprotos.h"
#include "utils/sortsupport.h"
#include "utils/varlena.h"

#define BINARY_DEFAULT_LEN  1

static Datum 
copy_binary(Datum source, int typmod, bool target_is_var)
{
	int maxlen = typmod - (int32)VARHDRSZ;
	int length = VARSIZE_ANY_EXHDR(source);
	char *data = NULL;
	char *rp = NULL;
	bytea *result = NULL;
	int alloc_size = 0;
	struct varlena *attr = (struct varlena *)DatumGetPointer(source);
	data = VARDATA_ANY(attr);

	alloc_size = (target_is_var ? length : (maxlen < 0 ? length : maxlen)) + VARHDRSZ;
	result = (bytea *)palloc0(alloc_size);
	SET_VARSIZE(result, alloc_size);

	if (length == 0)
	{
		PG_RETURN_POINTER(result);
	}
	
	rp = VARDATA(result);
	memcpy(rp, data, length);
	
	PG_RETURN_BYTEA_P(result);
}

PG_FUNCTION_INFO_V1(binary);
PG_FUNCTION_INFO_V1(binary2bool);
PG_FUNCTION_INFO_V1(binary_in);
PG_FUNCTION_INFO_V1(bytea_binary);
PG_FUNCTION_INFO_V1(binary_sortsupport);

Datum 
binary(PG_FUNCTION_ARGS)
{
	bytea *source = PG_GETARG_BYTEA_P(0);
	int32 typmod = PG_GETARG_INT32(1);
	bool isExplicit = PG_GETARG_BOOL(2);
	int32 length = VARSIZE_ANY_EXHDR(source);
	int32 maxlen = typmod - VARHDRSZ;

	if (typmod == VARHDRSZ)
	{
		if (length == 0)
		{
			PG_RETURN_NULL();
		}
		
		if (!isExplicit)
		{
			ereport(ERROR, 
					(errmsg("The input length:%d exceeds the maximum length:%d.", length, maxlen)));
		}
		
		ereport(WARNING, 
				(errmsg("The input length:%d exceeds the maximum length:%d, data truncated", length, maxlen)));
						
		PG_RETURN_BYTEA_P(truncate_bytea_with_len(source, maxlen));
	}

	if (maxlen > 0 && length > maxlen)
	{
		if (is_strict_mode() && !isExplicit)
		{
			ereport(ERROR, (errmsg("The input length:%d exceeds the maximum length:%d.", length, maxlen)));
		}
		else
		{
			ereport(WARNING, (errmsg("The input length:%d exceeds the maximum length:%d, data truncated", length, maxlen)));
			PG_RETURN_BYTEA_P(truncate_bytea_with_len(source, maxlen));
		}
	}

	return copy_binary(PointerGetDatum(source), typmod, false);
}

Datum 
binary2bool(PG_FUNCTION_ARGS)
{
	return varbinary2bool(fcinfo);
}

Datum 
binary_in(PG_FUNCTION_ARGS)
{
	char 	*inputText = PG_GETARG_CSTRING(0);
	int32 	atttypmod = PG_GETARG_INT32(2),
			bc, cl;
	char 	*tp = NULL,
			*rp = NULL;
	size_t 	len = strlen(inputText),
			hex_len = 0,
			maxlen = atttypmod - VARHDRSZ;
	bytea 	*result;
	bool	is_insert = CALL_IN_INSERT();

	/* Recognize hex input */
	/* Begin - bug#z5465 */ 
	if (inputText[0] == '\\' && inputText[1] == 'x')
	{
		hex_len = len - 2;
		if (atttypmod < VARHDRSZ)
		{
			bc = HEX_LEN(len) + VARHDRSZ; /* maximum possible length */
			result = (bytea *) palloc(bc);
		}
		else
		{
			if (HEX_LEN(len) > maxlen)
			{
				/* Begin - bug#z5800 */
				if (is_insert)
				{
					if (is_strict_mode())
					{
						ereport(ERROR,
								(errmsg("Data too long for column")));
					}
					else
					{
						ereport(WARNING,
								(errmsg("Data truncated for column")));
						
						hex_len = maxlen * 2;
					}
				}
				else
				{
					ereport(WARNING,
							(errmsg("Truncated incorrect BINARY(%d) value: (%s)", atttypmod - VARHDRSZ, inputText)));

					hex_len = maxlen * 2;
				}
				/* End - bug#z5800 */
			}

			result = (bytea *) palloc0(atttypmod); /* palloc0, pad with zero */
		}

		bc = hex_decode(inputText + 2, hex_len, VARDATA(result));
		SET_VARSIZE(result, atttypmod < VARHDRSZ ? bc + VARHDRSZ : atttypmod);

		PG_RETURN_BYTEA_P(result);
	}
	/* End - bug#z5465 */ 

	/* Else, it's the traditional escaped style */
	if (len > maxlen)
	{
		char *new_text;

		/* Begin - bug#z5800 */
		if (is_insert)
		{
			if (is_strict_mode())
			{
				ereport(ERROR,
						(errmsg("Data too long for column")));
			}
			else
			{
				ereport(WARNING,
						(errmsg("Data truncated for column")));
			}
		}
		else
		{
			ereport(WARNING,
					(errmsg("Truncated incorrect BINARY(%d) value: (%s)", atttypmod - VARHDRSZ, inputText)));			
		}
		/* End - bug#z5800 */

		new_text = pnstrdup(inputText, maxlen);

		return DirectFunctionCall1(byteain, CStringGetDatum(new_text));
	}

	bc = 0;
	tp = inputText;

	while (*tp != '\0')
	{
		if (tp[0] != '\\')
		{
			cl = pg_mblen(tp);
			tp += cl;
			bc += cl;
		}
		else
		{
			switch (tp[1])
			{
				case '%':
					tp += 2;
					bc += 2;
					break;
				case '_':
					tp += 2;
					bc += 2;
					break;
				case '\0':
					tp += 1;
					bc++;
					break;
				default:
					tp += 2;
					bc++;
					break;
			}
		}
	}

	if (atttypmod < VARHDRSZ)
	{
		bc += VARHDRSZ;
		result = (bytea *) palloc(bc);
		SET_VARSIZE(result, bc);
	}
	else
	{
		if (bc > atttypmod - VARHDRSZ)
		{
			ereport(ERROR,
					(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
					 errmsg("value too long for type binary(%d)", atttypmod - VARHDRSZ)));
		}

		result = (bytea *) palloc0(atttypmod); /* palloc0, pad with zero */
		SET_VARSIZE(result, atttypmod);
	}

	tp = inputText;
	rp = VARDATA(result);
	
	while (*tp != '\0')
	{
		if (tp[0] != '\\')
		{
			cl = pg_mblen(tp);
			for (int i = 0; i < cl; i++)
				*rp++ = *tp++;
		}
		else
		{
			switch (tp[1])
			{
				case '\\':
					*rp++ = '\\';
					tp += 2;
					break;
				case '0':
					*rp++ = VAL('0');
					tp += 2;
					break;
				case 't':
					*rp++ = '\t';
					tp += 2;
					break;
				case 'b':
					*rp++ = '\b';
					tp += 2;
					break;
				case 'n':
					*rp++ = '\n';
					tp += 2;
					break;
				case 'r':
					*rp++ = '\r';
					tp += 2;
					break;
				case '\'':
					*rp++ = '\'';
					tp += 2;
					break;
				case '\"':
					*rp++ = '\"';
					tp += 2;
					break;
				case '%':
					*rp++ = '\\';
					*rp++ = '%';
					tp += 2;
					break;
				case '_':
					*rp++ = '\\';
					*rp++ = '_';
					tp += 2;
					break;
				case '\0':
					*rp++ = '\\';
					tp += 1;
					break;
				default:
					*rp++ = tp[1];
					tp += 2;
					break;
			}
		}
	}

	PG_RETURN_BYTEA_P(result);
}

Datum 
bytea_binary(PG_FUNCTION_ARGS)
{
	bytea *source = PG_GETARG_BYTEA_P(0);
	int32 maxlen = PG_GETARG_INT32(1);
	if (maxlen == VARHDRSZ)
	{
		int length = VARSIZE_ANY_EXHDR(source);
		if (length == 0)
			PG_RETURN_NULL();
		else
			ereport(ERROR, (errmsg("The input length:%d exceeds the maximum length:%d.", length, maxlen)));
	}

	return copy_binary(PointerGetDatum(source), maxlen, false);
}

/* reference from 'Datum bytea_sortsupport(PG_FUNCTION_ARGS)' */
Datum 
binary_sortsupport(PG_FUNCTION_ARGS)
{
	SortSupport ssup = (SortSupport)PG_GETARG_POINTER(0);
	MemoryContext oldcontext;

	oldcontext = MemoryContextSwitchTo(ssup->ssup_cxt);

	/* Use generic string SortSupport, forcing "C" collation */
	varstr_sortsupport(ssup, BINARYOID, C_COLLATION_OID);

	MemoryContextSwitchTo(oldcontext);

	PG_RETURN_VOID();
}
