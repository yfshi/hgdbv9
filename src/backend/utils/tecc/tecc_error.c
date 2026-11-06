/*-------------------------------------------------------------------------
 *
 * tecc_error.c
 *	  Error handling implementation for TECC (全密态数据库) module
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/backend/utils/tecc/tecc_error.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include <string.h>

#include "storage/ipc.h"
#include "utils/elog.h"
#include "utils/tecc/tecc_error.h"

/* Begin - ReqID:TECC4.0 */

/*
 * tecc_report_error - Unified error reporting function for TECC module
 *
 * This function provides consistent error reporting with detailed context
 * information for debugging and user guidance.
 */
void
tecc_report_error(int sqlstate, const char *message,
				  const char *detail, const char *hint,
				  int lineno, const char *funcname)
{
	/* Prepare error report arguments */
	ErrorData  *edata;
	int			save_errno = errno;

	/* Check if we're already in an error context */
	if (in_error_recursion_trouble())
	{
		/*
		 * Oops, error while processing error. Just emit the message
		 * to stderr and abort to avoid infinite recursion.
		 */
		write_stderr("TECC error while processing error:\n");
		if (message)
			write_stderr("Message: %s\n", message);
		if (detail)
			write_stderr("Detail: %s\n", detail);
		if (hint)
			write_stderr("Hint: %s\n", hint);
		write_stderr("Location: %s:%d\n", funcname, lineno);
		proc_exit(1);
	}

	/* Initialize error data */
	edata = (ErrorData *) palloc0(sizeof(ErrorData));
	edata->elevel = ERROR;
	edata->sqlerrcode = sqlstate;
	edata->message = pstrdup(message);

	if (detail)
		edata->detail = pstrdup(detail);

	if (hint)
		edata->hint = pstrdup(hint);

	edata->filename = __FILE__;
	edata->lineno = lineno;
	edata->funcname = pstrdup(funcname);

	/* Restore errno in case it was changed by palloc/pstrdup */
	errno = save_errno;

	/* Throw the error */
	ThrowErrorData(edata);
}

/* End - ReqID:TECC4.0 */