/*-------------------------------------------------------------------------
 *
 * auth.h
 *	  Definitions for network authentication routines
 *
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/libpq/auth.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef AUTH_H
#define AUTH_H

#include "libpq/libpq-be.h"

extern char *pg_krb_server_keyfile;
extern bool pg_krb_caseins_users;
extern char *pg_krb_realm;

extern void ClientAuthentication(Port *port);

/* Hook for extensions that must reject a connection before authentication. */
typedef void (*ClientPreAuthentication_hook_type) (Port *port);
extern PGDLLIMPORT ClientPreAuthentication_hook_type ClientPreAuthentication_hook;

/* Hook for plugins to get control in ClientAuthentication() */
typedef void (*ClientAuthentication_hook_type) (Port *, int);
extern PGDLLIMPORT ClientAuthentication_hook_type ClientAuthentication_hook;

/* ReqID:SRS-SECURITY-IDENTITYAUTH */
extern PGDLLIMPORT char *ip_addr_str;
#endif							/* AUTH_H */
