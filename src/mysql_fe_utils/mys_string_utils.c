/*-------------------------------------------------------------------------
 *
 * String-processing utility routines for frontend code (MySQL Compatibility)
 *
 * Assorted utility functions that are useful in constructing SQL queries
 * and interpreting backend output.
 *
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/mysql_fe_utils/mys_string_utils.c
 *
 * add the file for requirement "SRS-MISC-MYSQL-FRAME"
 *
 *-------------------------------------------------------------------------
 */
#include "postgres_fe.h"

#include <ctype.h>

#include "fe_utils/string_utils.h"

#include "mysql_fe_utils/mys_string_utils.h"

#include "mysql_parser/mys_keywords.h"
#include "libpq-fe.h"
#include "mb/pg_wchar.h"



static PQExpBuffer mysDefaultGetLocalPQExpBuffer(void);


/*
 * Returns a temporary PQExpBuffer, valid until the next call to the function.
 * This is used by ora_fmtId and fmtQualifiedId.
 *
 * Non-reentrant and non-thread-safe but reduces memory leakage. You can
 * replace this with a custom version by setting the oraGetLocalPQExpBuffer
 * function pointer.
 */
static PQExpBuffer
mysDefaultGetLocalPQExpBuffer(void)
{
	static PQExpBuffer id_return = NULL;

	if (id_return)				/* first time through? */
	{
		/* same buffer, just wipe contents */
		resetPQExpBuffer(id_return);
	}
	else
	{
		/* new buffer */
		id_return = createPQExpBuffer();
	}

	return id_return;
}

/*
 *	Quotes input string if it's not a legitimate SQL identifier as-is.
 *
 *	Note that the returned string must be used before calling fmtId again,
 *	since we re-use the same return buffer each time.
 */
const char *
mys_fmtId(const char *rawid, int encoding)
{
	PQExpBuffer id_return = mysDefaultGetLocalPQExpBuffer();

	const char *cp;
	bool		need_quotes = false;
	size_t		remaining = strlen(rawid);

	/*
	 * These checks need to match the identifier production in scan.l. Don't
	 * use islower() etc.
	 */
	if (quote_all_identifiers)
		need_quotes = true;
	/* slightly different rules for first character */
	else if (!((rawid[0] >= 'a' && rawid[0] <= 'z')
			|| rawid[0] == '_'
			|| rawid[0] == '"'))
		need_quotes = true;
	else if (rawid[0] != '"')
	{
		/* otherwise check the entire string */
		cp = rawid;
		for (size_t i = 0; i < remaining; i++, cp++)
		{
			if (!((*cp >= 'a' && *cp <= 'z')
				  || (*cp >= '0' && *cp <= '9')
				  || (*cp == '_')))
			{
				need_quotes = true;
				break;
			}
		}
	}

	if (!need_quotes)
	{
		/*
		 * Check for keyword.  We quote keywords except for unreserved ones.
		 * (In some cases we could avoid quoting a col_name or type_func_name
		 * keyword, but it seems much harder than it's worth to tell that.)
		 *
		 * Note: ScanKeywordLookup() does case-insensitive comparison, but
		 * that's fine, since we already know we have all-lower-case.
		 */
		int			kwnum = ScanKeywordLookup(rawid, &MysScanKeywords);

		if (kwnum >= 0 && MysScanKeywordCategories[kwnum] != UNRESERVED_KEYWORD)
			need_quotes = true;
	}

	if (!need_quotes)
	{
		/* no quoting needed */
		appendPQExpBufferStr(id_return, rawid);
	}
	else
	{
		appendPQExpBufferChar(id_return, '"');

		cp = &rawid[0];
		while (remaining > 0)
		{
			int			charlen;

			/* Fast path for plain ASCII */
			if (!IS_HIGHBIT_SET(*cp))
			{
				/*
				 * Did we find a double-quote in the string? Then make this a
				 * double double-quote per SQL99. Before, we put in a
				 * backslash/double-quote pair. - thomas 2000-08-05
				 */
				if (*cp == '"')
					appendPQExpBufferChar(id_return, '"');
				appendPQExpBufferChar(id_return, *cp);
				remaining--;
				cp++;
				continue;
			}

			/* Slow path for possible multibyte characters */
			charlen = pg_encoding_mblen(encoding, cp);

			if (remaining < charlen)
			{
				/*
				 * If the character is longer than the available input,
				 * replace the string with an invalid sequence. The invalid
				 * sequence ensures that the escaped string will trigger an
				 * error on the server-side, even if we can't directly report
				 * an error here.
				 */
				enlargePQExpBuffer(id_return, 2);
				pg_encoding_set_invalid(encoding,
										id_return->data + id_return->len);
				id_return->len += 2;
				id_return->data[id_return->len] = '\0';

				/* there's no more input data, so we can stop */
				break;
			}
			else if (pg_encoding_verifymbchar(encoding, cp, charlen) == -1)
			{
				/*
				 * Multibyte character is invalid.  It's important to verify
				 * that as invalid multi-byte characters could e.g. be used to
				 * "skip" over quote characters, e.g. when parsing
				 * character-by-character.
				 *
				 * Replace the bytes corresponding to the invalid character
				 * with an invalid sequence, for the same reason as above.
				 *
				 * It would be a bit faster to verify the whole string the
				 * first time we encounter a set highbit, but this way we can
				 * replace just the invalid characters, which probably makes
				 * it easier for users to find the invalidly encoded portion
				 * of a larger string.
				 */
				enlargePQExpBuffer(id_return, 2);
				pg_encoding_set_invalid(encoding,
										id_return->data + id_return->len);
				id_return->len += 2;
				id_return->data[id_return->len] = '\0';

				/*
				 * Copy the rest of the string after the invalid multi-byte
				 * character.
				 */
				remaining -= charlen;
				cp += charlen;
			}
			else
			{
				for (int i = 0; i < charlen; i++)
				{
					appendPQExpBufferChar(id_return, *cp);
					remaining--;
					cp++;
				}
			}
		}
		appendPQExpBufferChar(id_return, '"');
	}

	return id_return->data;
}

/* Begin - ReqID:SRS-MYSQL-CASE */
void get_mysql_objname(PGconn *conn, Oid oid, char **objname, char **nspname)
{
	PQExpBufferData		tmpbuf;
	PGresult		   *res;

	initPQExpBuffer(&tmpbuf);

	if (!OidIsValid(oid))
		return;

	appendPQExpBuffer(&tmpbuf, "select * from pg_catalog.pg_object where oid = %d; \n", oid);
	res = PQexec(conn, tmpbuf.data);

	if (PQntuples(res) == 1)
	{
		char	   *tmpnspname = NULL;

		*objname = pg_strdup(PQgetvalue(res, 0, 1));
		tmpnspname = pg_strdup(PQgetvalue(res, 0, 4));
		if (tmpnspname)
		{
			PQExpBufferData		tmpbuf1;
			PGresult		   *schres;
			char			   *tmpstr = NULL;

			initPQExpBuffer(&tmpbuf1);

			appendPQExpBuffer(&tmpbuf1, "select objname from pg_catalog.pg_object where oid = %s::oid; \n", tmpnspname);
			schres = PQexec(conn, tmpbuf1.data);
			if (PQntuples(schres) == 1)
			{
				tmpstr = PQgetvalue(schres, 0, 0);

				if (tmpstr)
					*nspname = pg_strdup(tmpstr);
			}
			termPQExpBuffer(&tmpbuf1);
		}
	}
	termPQExpBuffer(&tmpbuf);
}
/* End - ReqID:SRS-MYSQL-CASE */
