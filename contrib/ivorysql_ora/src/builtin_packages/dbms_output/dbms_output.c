/*---------------------------------------------------------------------------------
 *
 * File:dbms_output.c
 *
 * Abstract:
 *		Internal implementation of DBMS_OUTPUT package
 *
 *
 * ReqID:SRS-PLSQL-DBMS_OUTPUT
 *
 * Authored by zhenmingyang@highgo.com,20250721.
 *
 * Copyright:
 * Portions Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  contrib/ivorysql_ora/src/builtin_packages/dbms_output/dbms_output.c
 *
 * This is a modification of code taken from
 * https://github.com/orafce/orafce/blob/master/putline.c
 * the original copyright notice follows:
 *
 * 0-clause license ("Zero Clause BSD")
 *
 * Copyright (C) 2008-2023 by Pavel Stehule <pavel.stehule@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose
 * with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 *---------------------------------------------------------------------------------
 */

#include "postgres.h"
#include "funcapi.h"
#include "access/heapam.h"
#include "access/htup_details.h"
#include "catalog/pg_type.h"
#include "lib/stringinfo.h"

#undef USE_SSL
#undef ENABLE_GSS
#include "libpq/libpq.h"
#include "libpq/pqformat.h"
#include "utils/memutils.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"


#if defined(WIN32) && !defined(_MSC_VER)
extern PGDLLIMPORT ProtocolVersion FrontendProtocol;	/* for mingw */
#endif

#define BUFSIZE_DEFAULT		20000
#define BUFSIZE_MIN			2000
#define BUFSIZE_MAX			1000000
#define BUFSIZE_UNLIMITED	-1	/* no limit */
#define LINESIZE_MAX		32767

static char	*buffer = NULL;
static int	buffer_size = 0;	/* allocated bytes in buffer */
static int	buffer_len = 0;		/* used bytes in buffer */
static int	buffer_get = 0;		/* retrieved bytes in buffer */

static int	buffer_end = 0;						/* position of the last newline in the buffer */
static int	newline_cnt = 0;					/* how many newlines */
static bool	buffer_need_reset = false;			/* reset buffer flag, mainly used for send_buffer */
static int	buffer_limit = BUFSIZE_UNLIMITED;	/* the size of the buffer specified by DBMS_OUTPUT */
MemoryContext dbms_output_mcontext = NULL;
/*
 * After buffer_limit changes, save the size of the new buffer.
 * this only work for limited buffer.
 */
static int	new_buffer_limit = 0;

static void add_str(const char *str, int len, bool is_newline);
static void add_newline(void);
static void send_buffer(void);
static void dbms_output_enable_internal(int32 n_buf_size);
static void dbms_output_disable_internal();
static void dbms_output_input_internal(FunctionCallInfo fcinfo, char *which);

void dbmsoutput_severoutput(bool, int32);
void dbmsoutput_sendbuffer(void);
void _dbms_output_PG_init(void);
void _dbms_output_PG_fini(void);

/* Saved hook value in case of unload */
static dbmsoutput_sendbuffer_hook_type pre_dbmsoutput_sendbuffer_hook = NULL;
static dbmsoutput_severoutput_hook_type pre_dbmsoutput_severoutput_hook = NULL;

PG_FUNCTION_INFO_V1(dbms_output_enable);
PG_FUNCTION_INFO_V1(dbms_output_disable);
PG_FUNCTION_INFO_V1(dbms_output_put);
PG_FUNCTION_INFO_V1(dbms_output_put_line);
PG_FUNCTION_INFO_V1(dbms_output_new_line);
PG_FUNCTION_INFO_V1(dbms_output_get_line);
PG_FUNCTION_INFO_V1(dbms_output_get_lines);


void _dbms_output_PG_init(void)
{
	pre_dbmsoutput_sendbuffer_hook = dbmsoutput_sendbuffer_hook;
	pre_dbmsoutput_severoutput_hook = dbmsoutput_severoutput_hook;
	dbmsoutput_sendbuffer_hook = dbmsoutput_sendbuffer;
	dbmsoutput_severoutput_hook = dbmsoutput_severoutput;
}

void _dbms_output_PG_fini(void)
{
	dbmsoutput_sendbuffer_hook = pre_dbmsoutput_sendbuffer_hook;
	dbmsoutput_severoutput_hook = pre_dbmsoutput_severoutput_hook;
	if (dbms_output_mcontext)
		MemoryContextDelete(dbms_output_mcontext);
}


/*
 * Aux. buffer functionality
 */
static void
add_str(const char *str, int len, bool is_newline)
{
	/* Discard all buffers if get_line was called. */
	if (buffer_get > 0 || buffer_need_reset)
	{
		buffer_get = 0;
		buffer_len = 0;
		buffer_end = 0;
		newline_cnt = 0;
		buffer_need_reset = false;

		/*
		 * When the buffer is unlimited, a large amount of data (example, 1GB)
		 * may be cached by calling PUT/PUT_LINE. Oracle may be larger, but we
		 * give it an upper limit, which is indeed different from Oracle. If 
		 * this happens to multiple backends at the same time, it will seriously
		 * waste memory because the size of the buffer required next time may be
		 * very small.
		 *
		 * Here we give users a control option to choose when to release memory
		 * based on the actual size of their physical memory.
		 */
		if (buffer_size < dbms_output_discard_buffer_limit)
		{
			if (buffer_limit != BUFSIZE_UNLIMITED && new_buffer_limit > 0)
				buffer_limit = new_buffer_limit;
			new_buffer_limit = 0;
		}
		else
		{
			int	rpsize;

			/* release memory to OS */
			pfree(buffer);

			buffer_limit = new_buffer_limit > 0 ? new_buffer_limit : buffer_limit;
			new_buffer_limit = 0;
			if (buffer_limit == BUFSIZE_UNLIMITED)
				rpsize = ALLOCSET_DEFAULT_INITSIZE;
			else if (buffer_limit >= BUFSIZE_DEFAULT)
				rpsize = BUFSIZE_MAX;
			else
				rpsize = BUFSIZE_DEFAULT;

			buffer = MemoryContextAlloc(dbms_output_mcontext, rpsize + 2);
			buffer_size = rpsize;
		}
	}

	/* Compatible Oracle: limit does not include newlines */
	if (!is_newline &&
		buffer_limit != BUFSIZE_UNLIMITED &&
		buffer_len - newline_cnt + len > buffer_limit)
		ereport(ERROR,
			(errcode(ERRCODE_INSUFFICIENT_RESOURCES),
			 errmsg("buffer overflow, limit of %d bytes", 
			 new_buffer_limit ? new_buffer_limit : buffer_limit)));

	/*
	 * The maximum line size is 32767 bytes, and even if the size of the row
	 * added this time exceeds 32767 , the previously added rows that do not
	 * exceed 32767 are still valid in the buffer.
	 *
	 * Note, the maximum line size(32767) does not include the trailing newline.
	 */
	if (!is_newline && (buffer_len + len - buffer_end) > LINESIZE_MAX)
		ereport(ERROR,
			(errcode(ERRCODE_INSUFFICIENT_RESOURCES),
			 errmsg("line length overflow, limit of 32767 bytes per line")));

	/* enlarge buffer */
	if (buffer_len + len > buffer_size)
	{
		Size	rpsize;

		rpsize = buffer_size * 2;
		
		while(rpsize < buffer_len + len)
			rpsize *= 2;

		if (rpsize > MaxAllocSize - 2)
			rpsize = buffer_len + len;

		buffer = repalloc(buffer, rpsize + 2);
		buffer_size = rpsize;
	}

	memcpy(buffer + buffer_len, str, len);
	buffer_len += len;
	buffer[buffer_len] = '\0';

	/*
	 * To be compatible with Oracle, we save the position of the last newline character
	 * so that when refreshing the buffer, the content without a newline character will
	 * be discarded, that is, the content that the PUT interface inserted into the buffer
	 * but did not call NEW_LINE.
	 */
	if (is_newline)
	{
		buffer_end = buffer_len;
		newline_cnt++;
	}
}

static void
add_newline(void)
{
	add_str("", 1, true);	/* add \0 */
}

/*
 * Flush the entire buffer and reset all buffer related variables.
 * this is the underlying routine that flushes the buffer when SET
 * SERVEROUTPUT ON is turned on. After testing, when SERVEROUTPUT
 * is turned on, it is equivalent to calling an interface similar
 * to GET_LINE at the end of the procedure, so the content of the
 * buffer without new lines will not be retained , so we need to
 * reset the buffer here.
 */
static void
send_buffer()
{
	if (buffer_len > 0 &&
		buffer_get < buffer_end)
	{
		StringInfoData msgbuf;
		char *cursor = buffer + buffer_get;
		int	send_len = buffer_end - buffer_get;
		char	*line_start = cursor;
		StringInfo	result = makeStringInfo();

		while (--send_len > 0)
		{
			/*
			 * If there are multiple lines of content to be sent,
			 * the newline character of each line is actually '\0'
			 * instead of '\n', so in order not to truncate the content
			 * to be sent, we process the newline character here.
			 */
			if (*cursor == '\0')
			{
				//*cursor = '\n';
				/* Compatible Oracle: empty lines are not sent to the client */
				if (cursor - line_start > 0)
				{
					appendStringInfoString(result, line_start);
					appendStringInfoChar(result, '\n');
				}
				line_start = cursor + 1;
			}
			cursor++;
		}

		if (*cursor != '\0')
			ereport(ERROR,
				    (errcode(ERRCODE_INTERNAL_ERROR),
				     errmsg("internal error"),
				     errdetail("Wrong message format detected")));
		else
		{
			/* last line no need newline character */
			if (cursor - line_start > 0)
				appendStringInfoString(result, line_start);
		}

		if (result->len == 0)
		{
			buffer_get = buffer_end;
			pfree(result->data);
			pfree(result);
			return;
		}

		pq_beginmessage(&msgbuf, 'N');

		/*
		 * FrontendProtocol is not avalilable in MSVC because it is not
		 * PGDLLEXPORT'ed. So, we assume always the protocol >= 3.
		 */

#ifndef _MSC_VER

		if (PG_PROTOCOL_MAJOR(FrontendProtocol) >= 3)
		{

#endif

			pq_sendbyte(&msgbuf, PG_DIAG_MESSAGE_PRIMARY);
			pq_sendstring(&msgbuf, result->data);
			pq_sendbyte(&msgbuf, '\0');

#ifndef _MSC_VER

		}
		else
		{
			appendStringInfoChar(result, '\n');
			appendStringInfoChar(result, '\0');
			pq_sendstring(&msgbuf, result->data);
		}

#endif

		pq_endmessage(&msgbuf);
		pq_flush();

		buffer_get = buffer_end;
		pfree(result->data);
		pfree(result);
	}

	/*
	 * Reset buffer for compatible Oracle.
	 *
	 * TODO:
	 * there is no actual reset (pfree/MemoryContextReset) of the buffer performed here,
	 * but only the reset buffer flag is set. Therefore, if the current procedure call
	 * occupies a large buffer, then even if the current procedure is completed, it will
	 * not be actively released, but wait for the next call to GET_LINE.
	 *
	 * By checking the size of the memory (Session PGA Memory) used by Oracle's session
	 * process(from v$sesstat), the behavior is indeed as described above(Oracle Doc).
	 *
	 * I don't think this is a good idea, but for now, let's follow Oracle's behavior.
	 * maybe resetting the buffer directly here is a better choice.
	 */
	buffer_need_reset = true;	/* reset flag */
}

/*
 * The buffer_size parameter of the ENABLE interface of the Oracle DBMS_OUTPUT
 * package has two conditions: NULL and non-NULL. When non-NULL, the buffer size
 * is specified as a specific value. When NULL, it indicates that the buffer size
 * is not fixed and is unlimited. When ENABLE is called multiple times, 
 * the buffer_size is based on the last call.
 */
static void
dbms_output_enable_internal(int32 n_buf_size)
{
	if (dbms_output_mcontext == NULL)
	{
		dbms_output_mcontext = AllocSetContextCreate(TopMemoryContext,
												"DBMS_OUTPUT package context",
												ALLOCSET_DEFAULT_SIZES);
	}

	/* We allocate +2 bytes for an end-of-line and a string terminator. */
	if (buffer == NULL)
	{
		int		psize;

		/*
		 * n_buf_size cannot be greater than BUFSIZE_DEFAULT, see
		 * dbms_output_enable. in order to avoid the overhead of
		 * frequent memory allocation for newlines, we try to allocate
		 * enough memory here.
		 */
		if (n_buf_size == BUFSIZE_UNLIMITED)
			psize = ALLOCSET_DEFAULT_INITSIZE;
		else if (n_buf_size >= BUFSIZE_DEFAULT)
			psize = BUFSIZE_MAX;
		else
			psize = BUFSIZE_DEFAULT;

		buffer = MemoryContextAlloc(dbms_output_mcontext, psize + 2);
		buffer_size = psize;

		if (n_buf_size == BUFSIZE_UNLIMITED)
			buffer_limit = BUFSIZE_UNLIMITED;
		else
			buffer_limit = n_buf_size;

		buffer_len = 0;
		buffer_get = 0;
		buffer_end = 0;
		newline_cnt = 0;
		buffer_need_reset = false;
		new_buffer_limit = 0;
	}
	else
	{
		if (n_buf_size == BUFSIZE_UNLIMITED)
		{
			/*
			 * buffer_size from NULL or non-NULL converted to NULL
			 * this is simple we only need set the buffer_limit to
			 * BUFSIZE_UNLIMITED, memory for subsequent buffers is
			 * allocated on demand,and the contents of the old buffer
			 * are retained.
			 */
			buffer_limit = BUFSIZE_UNLIMITED;
			new_buffer_limit = 0;
		}
		else
		{
			/*
			 * the content that has been PUT/PUT_LINE to buffer is retained.
			 * if buffer is large enough, no repalloc is needed.
			 */
			if (buffer_size - buffer_len >= n_buf_size)
			{
				if (buffer_len > 0)
					new_buffer_limit = n_buf_size;
				buffer_limit = n_buf_size + buffer_len - newline_cnt;
			}
			else
			{
				int repalloc_szie;

				/* same as init memory allocate */
				if (n_buf_size >= BUFSIZE_DEFAULT)
					repalloc_szie = BUFSIZE_MAX;
				else
					repalloc_szie = BUFSIZE_DEFAULT;

				repalloc_szie += buffer_len;
				buffer = repalloc(buffer, repalloc_szie + 2);
				buffer_size = repalloc_szie;

				if (buffer_len > 0)
					new_buffer_limit = n_buf_size;

				buffer_limit = n_buf_size + buffer_len - newline_cnt;
			}
		}
	}
}

static void
dbms_output_disable_internal()
{
	if (dbms_output_mcontext)
	{
		MemoryContextReset(dbms_output_mcontext);
		buffer = NULL;
		buffer_size = 0;
		buffer_len = 0;
		buffer_get = 0;
		buffer_end = 0;
		newline_cnt = 0;
		buffer_need_reset = false;
		buffer_limit = BUFSIZE_UNLIMITED;
		new_buffer_limit = 0;
	}
}

static text *
dbms_output_next(void)
{
	/*
	 * Compatible with Oracle:
	 * reset the buffer regardless of whether have data is retrieved.
	 */
	buffer_need_reset = true;

	/* We will not output data without newline */
	if (buffer_get < buffer_end)
	{
		text *line = cstring_to_text(buffer + buffer_get);
		buffer_get += VARSIZE_ANY_EXHDR(line) + 1;
		return line;
	}
	else
		return NULL;
}

static void
dbms_output_input_internal(FunctionCallInfo fcinfo, char *which)
{
	Datum	arg = PG_GETARG_DATUM(0);
	Oid		arg_type = get_fn_expr_argtype(fcinfo->flinfo, 0);
	Oid		arg_outfunction = InvalidOid;
	bool	typIsVarlena;
	char	*arg_string = NULL;

	if (arg_type == InvalidOid)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("could not determine %s input data type", which)));

	arg_type = getBaseType(arg_type);
	getTypeOutputInfo(arg_type, &arg_outfunction, &typIsVarlena);
	arg_string = OidOutputFunctionCall(arg_outfunction, arg);

	/* empty string euqal to null */
	if (arg_string && strlen(arg_string) > 0)
	{
		add_str(arg_string, strlen(arg_string), false);
		pfree(arg_string);
	}
}


/*
 * HOOK INTERFACE.
 */
void
dbmsoutput_severoutput(bool enable, int32 size)
{
	if(enable)
		dbms_output_enable_internal(size);
	else
		dbms_output_disable_internal();
}

void 
dbmsoutput_sendbuffer(void)
{
	if(buffer)
		send_buffer();
}

/*
 * EXPORT INTERFACE.
 */
Datum
dbms_output_enable(PG_FUNCTION_ARGS)
{
	int32 n_buf_size;

	if (PG_ARGISNULL(0))
		n_buf_size = BUFSIZE_UNLIMITED;
	else
	{
		n_buf_size = PG_GETARG_INT32(0);

		if (n_buf_size > BUFSIZE_MAX)
			n_buf_size = BUFSIZE_MAX;
		else if (n_buf_size < BUFSIZE_MIN)
			n_buf_size = BUFSIZE_MIN;
	}

	dbms_output_enable_internal(n_buf_size);
	PG_RETURN_VOID();
}

Datum
dbms_output_disable(PG_FUNCTION_ARGS)
{
	dbms_output_disable_internal();
	PG_RETURN_VOID();
}

/*
 * main functions
 */
Datum
dbms_output_put(PG_FUNCTION_ARGS)
{
	/* PUT(NULL) do nothing */
	if (buffer && !PG_ARGISNULL(0))
		dbms_output_input_internal(fcinfo, "dbms_output.put");
	PG_RETURN_VOID();
}

Datum
dbms_output_put_line(PG_FUNCTION_ARGS)
{
	if (buffer)
	{
		/* PUT_LINE(NULL) == NEW_LINE() */
		if (!PG_ARGISNULL(0))
			dbms_output_input_internal(fcinfo, "dbms_output.put_line");
		add_newline();
	}
	PG_RETURN_VOID();
}


Datum
dbms_output_new_line(PG_FUNCTION_ARGS)
{
	if (buffer)
		add_newline();
	PG_RETURN_VOID();
}

Datum
dbms_output_get_line(PG_FUNCTION_ARGS)
{
	TupleDesc	tupdesc;
	HeapTuple	tuple;
	Datum		values[2];
	bool		nulls[2] = { false, false };
	text	   *line;

	/* Build a tuple descriptor for result type */
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");

	if ((line = dbms_output_next()) != NULL)
	{
		/* Compatible with Oracle: empty line return NULL instead of empty string */
		if (VARSIZE_ANY_EXHDR(line) == 0)
		{
			nulls[0] = true;
			values[1] = Int32GetDatum(0);
		}
		else
		{
			values[0] = PointerGetDatum(line);
			values[1] = Int32GetDatum(0);
		}
	}
	else
	{
		nulls[0] = true;
		values[1] = Int32GetDatum(1);
	}

	tuple = heap_form_tuple(tupdesc, values, nulls);

	PG_RETURN_DATUM(HeapTupleGetDatum(tuple));
}

Datum
dbms_output_get_lines(PG_FUNCTION_ARGS)
{
	TupleDesc	tupdesc;
	HeapTuple	tuple;
	Datum		values[2];
	bool		nulls[2] = { false, false };
	text	   *line;

	int32		max_lines = PG_GETARG_INT32(0);
	int32		n;
	ArrayBuildState *astate = NULL;

	/* Build a tuple descriptor for result type */
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");

	for (n = 0; n < max_lines && (line = dbms_output_next()) != NULL; n++)
	{
		/* Compatible with Oracle: empty line return NULL instead of empty string */
		if (VARSIZE_ANY_EXHDR(line) == 0)
			astate = accumArrayResult(astate,
									  (Datum) 0,
									  true,
									  TEXTOID,
									  CurrentMemoryContext);
		else
			astate = accumArrayResult(astate,
									  PointerGetDatum(line),
									  false,
									  TEXTOID,
									  CurrentMemoryContext);
	}

	if (n > 0)
		values[0] = makeArrayResult(astate, CurrentMemoryContext);
	else
		values[0] = PointerGetDatum(construct_empty_array(TEXTOID));
	values[1] = Int32GetDatum(n);
	tuple = heap_form_tuple(tupdesc, values, nulls);

	PG_RETURN_DATUM(HeapTupleGetDatum(tuple));
}
