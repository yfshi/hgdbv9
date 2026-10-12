/*-------------------------------------------------------------------------
 *
 * relation.c
 *	  routines about relation limit
 *
 *	  contrib/hg_alarm/relation.c
 *-------------------------------------------------------------------------
 */
#include "hg_alarm.h"

/*
 * Get the size and kind of a relation 
 *	dbname, nspname, relanme to id a perticular relation.
 *	return:
 *		true: successfuly get the size and kind of the relation, with *relsize and *relkind set.
 *		false: any error occured while get the size and kind of the relation.
 */
int64
get_rel_size_and_kind(PGconn *conn, char *dbname, char *nspname, char *relname, 
					  char *relkind)
{
	char 	sql[MAXSQLLEN] = {0};
	char	relkind_tmp;
	int64	relsize = 0L;
	PGresult	*res;
	char		*val;
	PGconn		*remote_conn = NULL;
	PGconn		*exec_conn;
	int			 ret = 0;

	if (strcmp(dbname, GlobalUserConfig.guc_database))
	{
		ConnParams		cp;

		cp.dbname = dbname;
		cp.pguser = GlobalUserConfig.guc_username;
		cp.pghost = GlobalUserConfig.guc_host;
		cp.pgport = GlobalUserConfig.guc_port;
		cp.password = GlobalUserConfig.guc_password;
		cp.prompt_password = TRI_NO;
		cp.override_dbname = NULL;

		exec_conn = remote_conn = connectDatabase(&cp, hga_progname, false, true, false, NULL);
		if (!exec_conn)
		{
			hga_log_warning("RELATION: cound not connect to database \"%s\"",
							dbname);
			return -1;
		}
	}
	else
		exec_conn = conn;
	
	if (!exec_conn)
		return -1;

	/* db exists? */
	snprintf(sql, MAXSQLLEN, 
			 "SELECT oid FROM pg_database WHERE datname = $$%s$$", 
			 dbname);
	res = PQexec(exec_conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK
		|| PQntuples(res) == 0)
	{
		hga_log_warning("RELATION: database \"%s\" does not exist", 
						dbname);
		ret = -2;
		goto error_return;
	}
	PQclear(res);
	
	/* schema exists? */
	snprintf(sql, MAXSQLLEN, 
			 "SELECT oid FROM pg_namespace WHERE nspname = $$%s$$", 
			 nspname);
	res = PQexec(exec_conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK
		|| PQntuples(res) == 0)
	{
		hga_log_warning("RELATION: schema \"%s\" of database \"%s\" does not exist", 
			 			nspname, dbname);
		ret = -3;
		goto error_return;
	}
	PQclear(res);

	/* rel exists? */
	snprintf(sql, MAXSQLLEN, 
			 "SELECT oid, relkind FROM pg_class WHERE relname = $$%s$$ AND "
			 "relnamespace IN "
			 "(SELECT oid FROM pg_namespace WHERE nspname=$$%s$$)", 
			 relname, nspname);
	res = PQexec(exec_conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK
		|| PQntuples(res) == 0)
	{
		hga_log_warning("RELATION: relation \"%s.%s\" of database \"%s\" does not exist",
			 			nspname, relname, dbname);
		ret = -4;
		goto error_return;
	}

	if (relkind)
	{
		val = PQgetvalue(res, 0, PQfnumber(res, "relkind"));
		relkind_tmp = val[0];
	}
	PQclear(res);

	/* get the real size of relation */
	snprintf(sql, sizeof(sql), 
			 "SELECT pg_relation_size('%s.%s')",
			 nspname, relname);
	res = PQexec(exec_conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		hga_log_warning("RELATION: %s",
						PQerrorMessage(exec_conn));
		ret = -5;
		goto error_return;
	}
	else
		relsize = atol(PQgetvalue(res, 0, 0));
	PQclear(res);

	/* disconnect the transiant connection */
	if (remote_conn)
		disconnectDatabase(remote_conn);

	/* Make sure relkind is set only if we got the relsize */
	if (relkind)
		*relkind = relkind_tmp;
	
	return relsize;
error_return:
	PQclear(res);
	/* disconnect the transiant connection */
	if (remote_conn)
		disconnectDatabase(remote_conn);

	return ret;
}

/*
 * Do relation checking.
 * 	if relname specified, only check this relaion, or null to check all.
 *	return:
 *		true: all relations size not exceed
 *		false: at least one relation size exceed or any error occured
 */
bool
do_rel_check(PGconn *conn, PGObject *rellist, char *relname)
{
	PGObject	*po;
	bool		 all_valid = true;
	bool		 rel_found = false;

	po = rellist;

	/* empty list */
	if (!po)
	{
		if (relname)
			goto rel_not_found;
		return true;
	}

	while (po)
	{
		int64		relsize_b;
		int32		relsize_mb;
		char		fullname[MAXPGPATH];

		snprintf(fullname, sizeof(fullname), "%s.%s.%s", 
				 po->object.rel.dbname,
				 po->object.rel.schema,
				 po->object.rel.relname);

		if (relname && strcmp(fullname, relname) != 0)
		{
			po = po->next;
			continue;
		}
		else
		{
			if (relname)
				rel_found = true;
		}

		relsize_b = get_rel_size_and_kind(conn,
										  po->object.rel.dbname,
										  po->object.rel.schema,
										  po->object.rel.relname,
										  NULL);
		if (relsize_b < 0)
		{
			if (relname)
				return false;
			else
			{
				all_valid = false;
				po = po->next;
				continue;
			}
		}

		/* Skip unlimited ones */
		if (po->object.rel.limit == UNLIMITED_INT)
		{
			if (relname)
				return true;
			po = po->next;
			continue;
		}
		
		relsize_mb = relsize_b / (1024 * 1024);

		if (relsize_mb > po->object.rel.limit)
		{
			hga_log_warning("RELATION: relation \"%s.%s.%s\" size[current: %dMB] exceeds[limit: %dMB]", 
							po->object.rel.dbname,
							po->object.rel.schema,
							po->object.rel.relname,
							relsize_mb, 
							po->object.rel.limit);
			if (relname)
				return false;
			all_valid = false;
		}
		else
		{
			if (relname)
				return true;
		}
		
		po = po->next;
	}

	if (relname && !rel_found)
		goto rel_not_found;

	return all_valid;

rel_not_found:
	hga_log_warning("RELATION: relation \"%s\" size is not limited, please make sure you have add it into "
				   "\"hg_alarm.relation_limit\"", 
					relname);
	return false;
}