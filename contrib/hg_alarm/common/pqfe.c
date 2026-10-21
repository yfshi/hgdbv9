/*-------------------------------------------------------------------------
 *
 * Facilities for frontend code to connect to and disconnect from databases.
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/fe_utils/connect_utils.c
 *
 *-------------------------------------------------------------------------
 */
#include "hg_alarm.h"

#include "common/string.h"
#include "common/username.h"

#include <unistd.h>

/*
 * Make a database connection with the given parameters.
 *
 * An interactive password prompt is automatically issued if needed and
 * allowed by cparams->prompt_password.
 *
 * If allow_password_reuse is true, we will try to re-use any password
 * given during previous calls to this routine.  (Callers should not pass
 * allow_password_reuse=true unless reconnecting to the same database+user
 * as before, else we might create password exposure hazards.)
 */
PGconn *
connectDatabase(ConnParams *cparams, const char *progname,
				bool echo, bool fail_ok, 
				bool save_password, GUC *guc)
{
	PGconn	   *conn;
	bool		new_pass;

	char       *password = NULL;

	/* Callers must supply at least dbname; other params can be NULL */
	Assert(cparams->dbname);

	#ifdef FRONTEND
	if (cparams->prompt_password == TRI_YES)
		password = simple_prompt("Password: ", false);
	else
	#endif

	password = cparams->password;

	/*
	 * Start the connection.  Loop until we have a password if requested by
	 * backend.
	 */
	do
	{
		const char *keywords[8];
		const char *values[8];
		int			i = 0;

		/*
		 * If dbname is a connstring, its entries can override the other
		 * values obtained from cparams; but in turn, override_dbname can
		 * override the dbname component of it.
		 */
		keywords[i] = "host";
		values[i++] = cparams->pghost;
		keywords[i] = "port";
		values[i++] = cparams->pgport;
		keywords[i] = "user";
		values[i++] = cparams->pguser;
		keywords[i] = "password";
		values[i++] = password;
		keywords[i] = "dbname";
		values[i++] = cparams->dbname;
		if (cparams->override_dbname)
		{
			keywords[i] = "dbname";
			values[i++] = cparams->override_dbname;
		}
		keywords[i] = "fallback_application_name";
		values[i++] = progname;
		keywords[i] = NULL;
		values[i++] = NULL;
		Assert(i <= lengthof(keywords));

		new_pass = false;
		conn = PQconnectdbParams(keywords, values, true);

		if (!conn)
		{
			hga_log_error("could not connect to database %s: out of memory",
						 cparams->dbname);
			if (fail_ok)
				return NULL;
			exit(1);
		}

		/*
		 * No luck?  Trying asking (again) for a password.
		 */
		if (PQstatus(conn) == CONNECTION_BAD &&
			PQconnectionNeedsPassword(conn))
		{
			#ifdef FRONTED
			if (cparams->prompt_password != TRI_NO)
			{
				PQfinish(conn);
				if (password)
					free(password);
				password = simple_prompt("Password: ", false);
				new_pass = true;
			}
			#endif
		}
	} while (new_pass);

	/* check to see that the backend connection was successfully made */
	if (PQstatus(conn) == CONNECTION_BAD)
	{
		if (fail_ok)
		{
			PQfinish(conn);
			return NULL;
		}
		hga_log_fatal("%s", PQerrorMessage(conn));
	}

	/* Json password is not valid, use the user-input password instead */
	if (save_password && guc)
    	guc->guc_password = password;

	return conn;
}

/*
 * Disconnect the given connection, canceling any statement if one is active.
 */
void
disconnectDatabase(PGconn *conn)
{
	char		errbuf[256];

	Assert(conn != NULL);

	if (PQtransactionStatus(conn) == PQTRANS_ACTIVE)
	{
		PGcancel   *cancel;

		if ((cancel = PQgetCancel(conn)))
		{
			(void) PQcancel(cancel, errbuf, sizeof(errbuf));
			PQfreeCancel(cancel);
		}
	}

	PQfinish(conn);
}


void
apply_default_connection_params(GUC *guc)
{
	/* 
	 * Try to use PGHOST/PGPORT env if host or port not specified
	 */
	if (!guc->guc_host)
	{
		guc->guc_host = getenv("PGHOST");
		if (!guc->guc_host)
			guc->guc_host = "localhost";
	}
	if (!guc->guc_port)
	{
		guc->guc_port = getenv("PGPORT");
		if (!guc->guc_port)
			guc->guc_port = pstrdup(DEF_PGPORT_STR);
	}

	/* database:
	 *	Must be highgo user for HGDB, or postgres for PG.
	 */
	if (!guc->guc_database)
	{
		#ifdef _PG_ALARM
		guc->guc_database = pstrdup("postgres");
		#else
		guc->guc_database = pstrdup("highgo");
		#endif
	}

	/* user:
	 *	Must be sysdba user for HGDB.
	 */
	if (!guc->guc_username)
	{
		#ifdef _PG_ALARM
		char	*name;

		name = get_user_name(NULL);
		if (!name)
			name = "postgres";
		guc->guc_username = pstrdup(name);
		#else
		guc->guc_username = pstrdup("sysdba");
		#endif
	}
}

void
valid_connection_param(GUC *guc)
{
	/*
	 * Database
	 */
	if (guc->guc_database)
	{
		#ifdef _PG_ALARM
		if (strcmp(guc->guc_database, "postgres"))
			hga_log_fatal("Can only specify \"postgres\" database.");
		#else
		if (strcmp(guc->guc_database, "highgo"))
			hga_log_fatal("Can only specify \"highgo\" database.");
		#endif
	}

	/*
	 * Username
	 */
//	if (guc->guc_username)
//	{
//		#ifdef _PG_ALARM
//		char *name = get_user_name(NULL);
//		if (!name)
//			name = "postgres";
//		if (strcmp(username_cmd, name))
//			hga_log_fatal("Can only specify \"%s\" user.", name);
//		#else
//		if (strcmp(guc->guc_username, "sysdba") && strcmp(guc->guc_username, "highgo"))
//			hga_log_fatal("Can only specify \"sysdba_role\" user.");
//		#endif
//	}
	/*
	 * Port
	 */
	if (guc->guc_port)
	{
		int32		 val;
		const char	*hintmsg;

		if (!parse_int(guc->guc_port, &val, 0, &hintmsg))
		{
			hga_log_info("invalid value: %s", guc->guc_port);
			if (hintmsg)
				hga_log_fatal("%s", hintmsg);
		}

		if ((val < 1) || (val > 65535))
			hga_log_fatal("invalid port number: %d", val);
	}
}