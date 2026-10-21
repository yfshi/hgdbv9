/*-------------------------------------------------------------------------
 *
 * database.c
 *	  routines about database limit
 *
 *	  contrib/hg_alarm/database.c
 *-------------------------------------------------------------------------
 */
#include "hg_alarm.h"

/*-----------------------
 *	database limit
 *-----------------------
 */
/*
 * Get the size of a certain database/tablespace.
 */
int64
get_db_or_ts_size(PGconn *conn, char *objname, int objtype)
{
	char 	sql[MAXSQLLEN] = {0};
	int64	objsize;
	PGresult	*res;
	char		*objfmt;
	char		*prefix;

	Assert(objtype == PGOBJ_DATABASE || objtype == PGOBJ_TABLESPACE);

	objfmt = (objtype == PGOBJ_DATABASE ? "database" : "tablespace");
	prefix = (objtype == PGOBJ_DATABASE ? "DATABASE" : "TABLESPACE");

	if (!conn)
		return -1;

	if (objtype == PGOBJ_DATABASE)
		snprintf(sql, MAXSQLLEN, 
				"SELECT * FROM pg_database WHERE datname = $$%s$$", 
				objname);
	else if (objtype == PGOBJ_TABLESPACE)
		snprintf(sql, MAXSQLLEN, 
				"SELECT * FROM pg_tablespace WHERE spcname = $$%s$$", 
				objname);
	else
		Assert(false);
	
	res = PQexec(conn, sql);
	if (!res 
		|| PQresultStatus(res) != PGRES_TUPLES_OK
		|| PQntuples(res) == 0)
	{
		PQclear(res);
		hga_log_warning("%s: %s \"%s\" does not exist",
				 		prefix, objfmt, objname);
		return -2;
	}

	if (objtype == PGOBJ_DATABASE)
		snprintf(sql, sizeof(sql), 
				 "SELECT pg_database_size($$%s$$)", 
				 objname);
	else if (objtype == PGOBJ_TABLESPACE)
		snprintf(sql, sizeof(sql), 
				 "SELECT pg_tablespace_size($$%s$$)", 
				 objname);
	else
		Assert(false);

	res = PQexec(conn, sql);
	if (!res || 
		PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		PQclear(res);
		hga_log_warning("%s: %s", 
			 			prefix, PQerrorMessage(conn));
		return -3;
	}
	else
		objsize = atol(PQgetvalue(res, 0, 0));
	
	PQclear(res);

	return objsize;
}

/*
 * objtype: DATABASE or TABLESPACE
 * objname: if specified, check only this object, or null to check all
 * return:
 * 	true: all objects size not exceed
 *  false: any error occured or at least one object size exceed
 */
bool
do_db_or_ts_check(PGconn *conn, PGOjbectType objtype, PGObject *objlist, char *objname)
{
	PGObject	*po;
	char 		*objfmt;
	char		*prefix;
	bool		 all_valid = true;
	bool		 obj_found = false;

	Assert(objtype == PGOBJ_DATABASE || objtype == PGOBJ_TABLESPACE);

	objfmt = (objtype == PGOBJ_DATABASE ? "database" : "tablespace");
	prefix = (objtype == PGOBJ_DATABASE ? "DATABASE" : "TABLESPACE");

	/* empty list */
	if (!objlist)
	{
		if (objname)
			goto obj_not_found;
		return true;
	}

	po = objlist;

	while (po)
	{
		int64		objsize_b;
		int32		objsize_mb;

		char		*t_name = objtype == PGOBJ_DATABASE 
								? po->object.db.name
								: po->object.ts.name;

		int32		 t_limit = objtype == PGOBJ_DATABASE 
								? po->object.db.limit
								: po->object.ts.limit;

		if (objname && strcmp(t_name, objname) != 0)
		{
			po = po->next;
			continue;
		}
		else
		{
			if (objname)
				obj_found = true;
		}

		objsize_b = get_db_or_ts_size(conn, t_name, objtype);
		if (objsize_b < 0)
		{
			if (objname)
				return false;
			else
			{
				all_valid = false;
				po = po->next;
				continue;
			}
		}

		/* Skip unlimited ones */
		if (t_limit == UNLIMITED_INT)
		{
			if (objname)
				return true;
			po = po->next;
			continue;
		}
		
		objsize_mb = objsize_b / (1024 * 1024);

		if (objsize_mb > t_limit)
		{
			hga_log_warning("%s: %s \"%s\" size[current: %dMB] exceeds[limit: %dMB]", 
							prefix, objfmt, 
							t_name, 
							objsize_mb,
							t_limit);
			if (objname)
				return false;
			all_valid = false;
		}
		else
		{
			if (objname)
				return true;
		}

		po = po->next;
	}

	if (objname && !obj_found)
		goto obj_not_found;
		
	return all_valid;

obj_not_found:
	hga_log_warning("%s: %s \"%s\" size is not limited, please make sure you have add it into "
					"\"hg_alarm.%s_limit\"", 
					prefix, objfmt, objname, objfmt);
	return false;
}