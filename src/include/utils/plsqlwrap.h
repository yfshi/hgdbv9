/* Begin - Reqid:SRS-PLPGSQL-HIDDEN*/
/*-------------------------------------------------------------------------
 *
 * plsqlwrap.h
 *	  PL/SQL source wrapping support.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PLSQLWRAP_H
#define PLSQLWRAP_H

#include "postgres.h"
#include <ctype.h>
#include "common/plsqlwrap_crypto.h"

/*
 * Return true when a source string starts with the IvorySQL wrapped marker.
 * Leading whitespace is ignored because the Oracle parser may preserve the
 * whitespace between IS/AS and the wrapped payload.
 */
static inline bool
ivy_plsql_source_is_wrapped(const char *src)
{
	if (src == NULL)
		return false;

	while (isspace((unsigned char) *src))
		src++;

	return strncmp(src, PG_PLSQL_WRAP_MAGIC, strlen(PG_PLSQL_WRAP_MAGIC)) == 0;
}

extern char *ivy_plsql_wrap_source(const char *src);
extern char *ivy_plsql_unwrap_source(const char *src);
extern char *ivy_plsql_wrap_ddl(const char *ddl);

#endif							/* PLSQLWRAP_H */
/* End - Reqid:SRS-PLPGSQL-HIDDEN*/
