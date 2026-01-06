/*-------------------------------------------------------------------------
 *
 * scansup.c
 *	  scanner support routines used by the core lexer
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/parser/scansup.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <ctype.h>

#include "mb/pg_wchar.h"
#include "parser/scansup.h"

/* Begin - ReqID:SRS-SECURITY-RESOURCEMANAGEMENT */
#include "utils/resgroup.h"
#include "utils/resource_manager.h"
/* End - ReqID:SRS-SECURITY-RESOURCEMANAGEMENT */

/*
 * downcase_truncate_identifier() --- do appropriate downcasing and
 * truncation of an unquoted identifier.  Optionally warn of truncation.
 *
 * Returns a palloc'd string containing the adjusted identifier.
 *
 * Note: in some usages the passed string is not null-terminated.
 *
 * Note: the API of this function is designed to allow for downcasing
 * transformations that increase the string length, but we don't yet
 * support that.  If you want to implement it, you'll need to fix
 * SplitIdentifierString() in utils/adt/varlena.c.
 */
char *
downcase_truncate_identifier(const char *ident, int len, bool warn)
{
	/* Begin - ReqID:SRS-SECURITY-RESOURCEMANAGEMENT */
	if(IsResGroupActivated() && pg_resource_group_disable_notice_scansup)
	{
		return downcase_identifier(ident, len, false, true);
	}
	else 
	{
		return downcase_identifier(ident, len, warn, true);
	}
	/* End - ReqID:SRS-SECURITY-RESOURCEMANAGEMENT */
}

/*
 * a workhorse for downcase_truncate_identifier
 */
char *
downcase_identifier(const char *ident, int len, bool warn, bool truncate)
{
	char	   *result;
	int			i;
	bool		enc_is_single_byte;
	Assert(ident != NULL);
	Assert(len >= 0);

	result = palloc(len + 1);
	enc_is_single_byte = pg_database_encoding_max_length() == 1;

	/*
	 * SQL99 specifies Unicode-aware case normalization, which we don't yet
	 * have the infrastructure for.  Instead we use tolower() to provide a
	 * locale-aware translation.  However, there are some locales where this
	 * is not right either (eg, Turkish may do strange things with 'i' and
	 * 'I').  Our current compromise is to use tolower() for characters with
	 * the high bit set, as long as they aren't part of a multi-byte
	 * character, and use an ASCII-only downcasing for 7-bit characters.
	 */
	memcpy(result, ident, len);
	for (i = 0; i < len; i++)
	{
		unsigned char ch = (unsigned char) ident[i];
		/* Begin - BUG#Z6063 */
		if (!enc_is_single_byte)
		{
			int mblen = pg_mblen(ident + i);
			if (mblen > 1)
			{
				i += (mblen - 1);
				continue;
			}
		}
		/* End - BUG#Z6063 */

		if (ch >= 'A' && ch <= 'Z')
			ch += 'a' - 'A';
		else if (enc_is_single_byte && IS_HIGHBIT_SET(ch) && isupper(ch))
			ch = tolower(ch);
		result[i] = (char) ch;
	}
	result[i] = '\0';

	if (i >= NAMEDATALEN && truncate)
		truncate_identifier(result, i, warn);

	return result;
}


/* BEGIN - case sensitive indentify */
char *
upcase_identifier(const char *ident, int len, bool warn, bool truncate)
{
	char	   *result;
	int			i;
	bool		enc_is_single_byte;
	Assert(ident != NULL);
	Assert(len >= 0);

	result = palloc(len + 1);
	enc_is_single_byte = pg_database_encoding_max_length() == 1;

	/*
	 * SQL99 specifies Unicode-aware case normalization, which we don't yet
	 * have the infrastructure for.  Instead we use toupper() to provide a
	 * locale-aware translation.  However, there are some locales where this
	 * locale-aware translation.  However, there are some locales where this
	 * is not right either (eg, Turkish may do strange things with 'i' and
	 * 'I').  Our current compromise is to use toupper() for characters with
	 * the high bit set, as long as they aren't part of a multi-byte
	 * character, and use an ASCII-only downcasing for 7-bit characters.
	 */
	memcpy(result, ident, len);
	for (i = 0; i < len; i++)
	{
		unsigned char ch = (unsigned char) ident[i];
		/* Begin - BUG#Z6063 */
		if (!enc_is_single_byte)
		{
			int mblen = pg_mblen(ident + i);
			if (mblen > 1)
			{
				i += (mblen - 1);
				continue;
			}
		}
		/* End - BUG#Z6063 */

		if (ch >= 'a' && ch <= 'z')
			ch -= 'a' - 'A';
		else if (enc_is_single_byte && IS_HIGHBIT_SET(ch) && islower(ch))
			ch = toupper(ch);
		result[i] = (char) ch;
	}
	result[i] = '\0';

	if (i >= NAMEDATALEN && truncate)
		truncate_identifier(result, i, warn);

	return result;
}


char *
identifier_case_transform(const char *ident, int len)
{
	char *upper_ident = NULL, *lower_ident = NULL, *result = NULL;

	upper_ident = upcase_identifier(ident, len, true, true);
	lower_ident = downcase_identifier(ident, len, true, true);

	if (strcmp(upper_ident, ident) == 0)
	{
		result = lower_ident;
		pfree(upper_ident);
	}
	else if (strcmp(lower_ident, ident) == 0)
	{
		result = upper_ident;
		pfree(lower_ident);
	}
	else
	{
		result = palloc0(len + 1);
		memcpy(result, ident, len);
		pfree(upper_ident);
		pfree(lower_ident);
	}

	return result;
}

/* Begin - BUG#Z1423 */
/*
 * Same as identifier_case_transform except that 
 * identifiers are not truncated according to the
 * length of NAMEDATALEN.
 */
char *
string_case_transform(const char *ident, int len)
{
	char *upper_ident = NULL, *lower_ident = NULL, *result = NULL;

	upper_ident = upcase_identifier(ident, len, false, false);
	lower_ident = downcase_identifier(ident, len, false, false);

	if (strcmp(upper_ident, ident) == 0)
	{
		result = lower_ident;
		pfree(upper_ident);
	}
	else if (strcmp(lower_ident, ident) == 0)
	{
		result = upper_ident;
		pfree(lower_ident);
	}
	else
	{
		result = palloc0(len + 1);
		memcpy(result, ident, len);
		pfree(upper_ident);
		pfree(lower_ident);
	}

	return result;
}
/* End - BUG#Z1423 */

/*
 * truncate_identifier() --- truncate an identifier to NAMEDATALEN-1 bytes.
 *
 * The given string is modified in-place, if necessary.  A warning is
 * issued if requested.
 *
 * We require the caller to pass in the string length since this saves a
 * strlen() call in some common usages.
 */
void
truncate_identifier(char *ident, int len, bool warn)
{
	if (len >= NAMEDATALEN)
	{
		len = pg_mbcliplen(ident, len, NAMEDATALEN - 1);
		if (warn)
			ereport(NOTICE,
					(errcode(ERRCODE_NAME_TOO_LONG),
					 errmsg("identifier \"%s\" will be truncated to \"%.*s\"",
							ident, len, ident)));
		ident[len] = '\0';
	}
}

/*
 * scanner_isspace() --- return true if flex scanner considers char whitespace
 *
 * This should be used instead of the potentially locale-dependent isspace()
 * function when it's important to match the lexer's behavior.
 *
 * In principle we might need similar functions for isalnum etc, but for the
 * moment only isspace seems needed.
 */
bool
scanner_isspace(char ch)
{
	/* This must match scan.l's list of {space} characters */
	if (ch == ' ' ||
		ch == '\t' ||
		ch == '\n' ||
		ch == '\r' ||
		ch == '\f')
		return true;
	return false;
}

/* Begin - ReqID:SRS-MYSQL-CASE */
char *
original_truncate_identifier(const char *ident, int len, bool warn)
{
	return original_identifier(ident, len, warn, true);
}

/*
 * a workhorse for original_truncate_identifier
 */
char *
original_identifier(const char *ident, int len, bool warn, bool truncate)
{
	char	   *result;
	int			i;

	result = palloc(len + 1);

	/*
	 * Keep the string output as it is, without case conversion.
	 */
	for (i = 0; i < len; i++)
	{
		unsigned char ch = (unsigned char) ident[i];

		result[i] = (char) ch;
	}
	result[i] = '\0';

	if (i >= NAMEDATALEN && truncate)
		truncate_identifier(result, i, warn);

	return result;
}
/* End - ReqID:SRS-MYSQL-CASE */

/*
 * Determine whether the letters in the string are all lowercase letters
 */
bool
identifier_is_all_lower(const char *ident, int len)
{
	int		i;
	const char	*s;

	s = ident;

	for (i = 0; i < len; i++)
	{
		/* Begin - BUG#Z6063 */
		int mblen = pg_mblen(s);

		if (mblen > 1)
		{
			s += mblen;
			i += (mblen - 1);
			continue;
		}
		/* End - BUG#Z6063 */

		if (isalpha(*s) && isupper(*s))
			return false;
		s++;
	}

	return true;
}

/*
 * Determine whether the letters in the string are all uppercase letters
 */
bool
identifier_is_all_upper(const char *ident, int len)
{
	int		i;
	const char	*s;

	s = ident;

	for (i = 0; i < len; i++)
	{
		/* Begin - BUG#Z6063 */
		int mblen = pg_mblen(s);

		if (mblen > 1)
		{
			s += mblen;
			i += (mblen - 1);
			continue;
		}
		/* End - BUG#Z6063 */

		if (isalpha(*s) && islower(*s))
			return false;
		s++;
	}

	return true;
}
