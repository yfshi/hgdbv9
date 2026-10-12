#include "hg_alarm.h"
#include "fmgr.h"

#include "miscadmin.h"
#include "nodes/execnodes.h"
#include "funcapi.h"
#include "se_features_interface.h"
#include "utils/acl.h"

PG_MODULE_MAGIC;

const char  *hga_progname = "hg_alarm";
GUC			 GlobalUserConfig;

void _PG_init(void);

void 
_PG_init(void)
{
	/* Nothing to do for now */
    return;
}

/*
 * Routine for a regular backend to check for a particular config:
 *	arg[0]: configuration file path
 *	arg[1]: which configuration to check
 */
PG_FUNCTION_INFO_V1(hg_alarm_check);
Datum
hg_alarm_check(PG_FUNCTION_ARGS)
{
	#define 	CHECK_ALL		1
	#define		CHECK_CONF		2

	char   *control_file = PG_GETARG_CSTRING(0);
	const char   *which = NULL;

	bool	res = true;
	int		mode = CHECK_ALL;

	char	*user_name;

	PGconn		*remote_conn = NULL;
	ConnParams	 cp;

	if (!superuser())
		elog(ERROR, 
			 "hg_alarm_check can only be called by sysdba_role user");
			
	user_name = GetUserNameFromId(GetUserId(), true);

	if (PG_NARGS() == 1)
		mode = CHECK_ALL;
	else if (PG_NARGS() == 2)
	{
		mode = CHECK_CONF;
		/*
		 * Only read the second argument when it is actually present.  The
		 * 1-argument overload has no second argument, and reading it here
		 * would dereference a NULL datum and crash the backend.
		 */
		which = PG_GETARG_CSTRING(1);
	}
	else
		Assert(false);
	
	memset(&GlobalUserConfig, 0, sizeof(GUC));

	if (!parse_json_option(control_file, &GlobalUserConfig))
		elog(ERROR, 
			"error while parsing configuration file \"%s\"", 
			control_file);
	
	apply_default_connection_params(&GlobalUserConfig);
	if (GlobalUserConfig.guc_username)
	{
		if (!IS_SYSDBA(get_role_oid(GlobalUserConfig.guc_username, false)))
			hga_log_fatal("Can only specified \"sysdba_role\" user.");
	}
	valid_connection_param(&GlobalUserConfig);

	cp.pghost = GlobalUserConfig.guc_host;
	cp.pgport = GlobalUserConfig.guc_port;
	cp.dbname = GlobalUserConfig.guc_database;
	cp.pguser = user_name;
	cp.password = GlobalUserConfig.guc_password;
	cp.prompt_password = false;
	cp.override_dbname = NULL;

	remote_conn = connectDatabase(&cp, hga_progname, false, true, false, NULL);
	if (!remote_conn)
		elog(ERROR, "cannot connect to database \"%s\"", GlobalUserConfig.guc_database);

	if (mode == CHECK_CONF)
	{
		if (strcasecmp(which, "memory_limit") == 0)
			res = do_memory_check(remote_conn, GlobalUserConfig.guc_memory_limit);
		else if (strcasecmp(which, "datadir_limit") == 0)
			res = do_datadir_check(remote_conn, GlobalUserConfig.guc_datadir_limit);
		else if (strcasecmp(which, "database_limit") == 0)
			res = do_db_or_ts_check(remote_conn, PGOBJ_DATABASE, GlobalUserConfig.guc_database_list, NULL);
		else if (strcasecmp(which, "tablespace_limit") == 0)
			res = do_db_or_ts_check(remote_conn, PGOBJ_TABLESPACE, GlobalUserConfig.guc_tablespace_list, NULL);
		else if (strcasecmp(which, "relation_limit") == 0
				|| strcasecmp(which, "table_limit") == 0
				|| strcasecmp(which, "index_limit") == 0)
			res = do_rel_check(remote_conn, GlobalUserConfig.guc_relation_list, NULL);
		else
		{
			disconnectDatabase(remote_conn);
			elog(ERROR, "unknown type '%s'", which);
		}
	}
	else
	{
		if(!do_memory_check(remote_conn, GlobalUserConfig.guc_memory_limit))
			res = false;
		if (!do_datadir_check(remote_conn, GlobalUserConfig.guc_datadir_limit))
			res = false;
		if (!do_db_or_ts_check(remote_conn, PGOBJ_DATABASE, GlobalUserConfig.guc_database_list, NULL))
			res = false;
		if (!do_db_or_ts_check(remote_conn, PGOBJ_TABLESPACE, GlobalUserConfig.guc_tablespace_list, NULL))
			res = false;
		if (!do_rel_check(remote_conn, GlobalUserConfig.guc_relation_list, NULL))
			res = false;
	}

	disconnectDatabase(remote_conn);

	PG_RETURN_BOOL(res);
}

/*
 * Routine for a regular backend to checkout the 
 * warning configs' stat.
 */
PG_FUNCTION_INFO_V1(hg_alarm_stat);
Datum
hg_alarm_stat(PG_FUNCTION_ARGS)
{
	char		*control_file = PG_GETARG_CSTRING(0);
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	MemoryContext per_query_ctx;
	MemoryContext oldcontext;
	TupleDesc tupleDesc;
	Tuplestorestate *tupleStore;
	ConnParams		 cp;
	PGconn			*remote_conn;

	Datum values[3];
	bool nulls[3];

	char	*user_name;

	if (!superuser())
		elog(ERROR, 
			 "hg_alarm_stat can only be called by sysdba user");

	user_name = GetUserNameFromId(GetUserId(), true);

	if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg
				 ("set-valued function called in context that cannot accept a set")));
	if (!(rsinfo->allowedModes & SFRM_Materialize))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("materialize mode required, but it is not "
						"allowed in this context")));

	per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
	oldcontext = MemoryContextSwitchTo(per_query_ctx);

	/*
	 * Build a tuple descriptor for our result type
	 */
	if (get_call_result_type(fcinfo, NULL, &tupleDesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");

	tupleStore = tuplestore_begin_heap(true, false, work_mem);
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupleStore;
	rsinfo->setDesc = tupleDesc;

	MemoryContextSwitchTo(oldcontext);

	if (!parse_json_option(control_file, &GlobalUserConfig))
		elog(ERROR, 
			"error while parsing configuration file \"%s\"", 
			control_file);

	apply_default_connection_params(&GlobalUserConfig);
	valid_connection_param(&GlobalUserConfig);

	cp.pghost = GlobalUserConfig.guc_host;
	cp.pgport = GlobalUserConfig.guc_port;
	cp.dbname = GlobalUserConfig.guc_database;
	cp.pguser = user_name;
	cp.password = GlobalUserConfig.guc_password;
	cp.prompt_password = false;
	cp.override_dbname = NULL;

	remote_conn = connectDatabase(&cp, hga_progname, false, true, false, NULL);
	if (!remote_conn)
		elog(ERROR, "cannot connect to database \"%s\"", GlobalUserConfig.guc_database);

	/* push memory limit tuple */
	do
	{
		int64	 current_kb;
		int32	 current_mb;
		char 	 b1[MAXINT8LEN + 3],
				 b2[MAXINT8LEN + 3]; /* plus the unit and '\0' */
		
		memset(nulls, 0, sizeof(nulls));
		memset(values, 0, sizeof(values));

		current_kb = get_memory_size(remote_conn);
		if (current_kb < 0)
			snprintf(b1, sizeof(b1), "-");
		else
		{
			current_mb = current_kb / 1024;
			snprintf(b1, sizeof(b1), "%d MB", current_mb);
		}
		if (GlobalUserConfig.guc_memory_limit == -1)
			snprintf(b2, sizeof(b2), "unlimited");
		else
			snprintf(b2, sizeof(b2), "%d MB", GlobalUserConfig.guc_memory_limit);
		values[0] = CStringGetDatum("memory_limit");
		values[1] = CStringGetDatum(b1);
		values[2] = CStringGetDatum(b2);
		tuplestore_putvalues(tupleStore, tupleDesc, values, nulls);
	} while(0);

	/* push datadir limit tuple */
	do
	{
		int64	 current;
		int32	 current_mb;
		char 	 b1[MAXINTLEN + 3],
				 b2[MAXINTLEN + 3]; /* plus the unit and '\0' */

		memset(nulls, 0, sizeof(nulls));
		memset(values, 0, sizeof(values));

		current = get_datadir_size(remote_conn);
		if (current < 0)
			snprintf(b1, sizeof(b1), "-");
		else
		{
			current_mb = current / (1024 * 1024);
			snprintf(b1, sizeof(b1), "%d MB", current_mb);
		}
		if (GlobalUserConfig.guc_datadir_limit == -1)
			snprintf(b2, sizeof(b2), "unlimited");
		else
			snprintf(b2, sizeof(b2), "%d MB", GlobalUserConfig.guc_datadir_limit);
		values[0] = CStringGetDatum("datadir_limit");
		values[1] = CStringGetDatum(b1);
		values[2] = CStringGetDatum(b2);
		tuplestore_putvalues(tupleStore, tupleDesc, values, nulls);
	} while(0);

	/* push database limit tuple(s) */
	if (GlobalUserConfig.guc_database_list)
	{
		PGObject	*po = GlobalUserConfig.guc_database_list;

		while (po)
		{
			int64		 current;
			int32		 current_mb;
			char 	 	 b1[MAXINTLEN + 3],
					 	 b2[MAXINTLEN + 3], /* plus the unit and '\0' */
						 db[NAMEDATALEN + 1];
			
			memset(nulls, 0, sizeof(nulls));
			memset(values, 0, sizeof(values));
			
			current = get_db_or_ts_size(remote_conn, po->object.db.name, PGOBJ_DATABASE);
			if (current < 0)
				snprintf(b1, sizeof(b1), "-");
			else
			{
				current_mb = current / (1024 * 1024);
				snprintf(b1, sizeof(b1), "%d MB", current_mb);
			}
			if (po->object.db.limit == -1) /* can't be here */
				snprintf(b2, sizeof(b2), "unlimited");
			else
				snprintf(b2, sizeof(b2), "%d MB", po->object.db.limit);
			snprintf(db, sizeof(db), "database_limit:%s", po->object.db.name);
			values[0] = CStringGetDatum(db);
			values[1] = CStringGetDatum(b1);
			values[2] = CStringGetDatum(b2);
			tuplestore_putvalues(tupleStore, tupleDesc, values, nulls);

			po = po->next;
		}
	}
	else
	{
		memset(nulls, 0, sizeof(nulls));
		memset(values, 0, sizeof(values));

		values[0] = CStringGetDatum("database_limit");
		values[1] = CStringGetDatum("null");
		values[2] = CStringGetDatum("null");
		tuplestore_putvalues(tupleStore, tupleDesc, values, nulls);
	}

	/* push tablespace limit tuple(s) */
	if (GlobalUserConfig.guc_tablespace_list)
	{
		PGObject	*po = GlobalUserConfig.guc_tablespace_list;

		while (po)
		{
			int64		 current;
			int32		 current_mb;
			char 	 	 b1[MAXINTLEN + 3],
					 	 b2[MAXINTLEN + 3], /* plus the unit and '\0' */
						 db[NAMEDATALEN + 1];
			
			memset(nulls, 0, sizeof(nulls));
			memset(values, 0, sizeof(values));
			
			current = get_db_or_ts_size(remote_conn, po->object.db.name, PGOBJ_TABLESPACE);
			if (current < 0)
				snprintf(b1, sizeof(b1), "-");
			else
			{
				current_mb = current / (1024 * 1024);
				snprintf(b1, sizeof(b1), "%d MB", current_mb);
			}
			if (po->object.db.limit == -1) /* can't be here */
				snprintf(b2, sizeof(b2), "unlimited");
			else
				snprintf(b2, sizeof(b2), "%d MB", po->object.db.limit);
			snprintf(db, sizeof(db), "tablespace_limit:%s", po->object.db.name);
			values[0] = CStringGetDatum(db);
			values[1] = CStringGetDatum(b1);
			values[2] = CStringGetDatum(b2);
			tuplestore_putvalues(tupleStore, tupleDesc, values, nulls);

			po = po->next;
		}
	}
	else
	{
		memset(nulls, 0, sizeof(nulls));
		memset(values, 0, sizeof(values));

		values[0] = CStringGetDatum("tablespace_limit");
		values[1] = CStringGetDatum("null");
		values[2] = CStringGetDatum("null");
		tuplestore_putvalues(tupleStore, tupleDesc, values, nulls);
	}
	
	/* push relation limit tuple(s) */
	if (GlobalUserConfig.guc_relation_list)
	{
		PGObject		*po = GlobalUserConfig.guc_relation_list;

		while (po)
		{
			char		 relkind = '-';
			int64		 current;
			int32		 current_mb;
			char 	 	 b1[MAXINTLEN + 3],
					 	 b2[MAXINTLEN + 3], /* plus the unit and '\0' */
						 fullname[NAMEDATALEN*3 + 1];
			
			memset(nulls, 0, sizeof(nulls));
			memset(values, 0, sizeof(values));
			
			current = get_rel_size_and_kind(remote_conn,
											po->object.rel.dbname, 
											po->object.rel.schema, 
											po->object.rel.relname,
											&relkind);
			if (current < 0)
				snprintf(b1, sizeof(b1), "-");
			else
			{
				current_mb = current / (1024 * 1024);
				snprintf(b1, sizeof(b1), "%d MB", current_mb);
			}
			if (po->object.rel.limit == -1) /* can't be here */
				snprintf(b2, sizeof(b2), "unlimited");
			else
				snprintf(b2, sizeof(b2), "%d MB", po->object.rel.limit);
			snprintf(fullname, sizeof(fullname),
					 "relation_limit:%s.%s.%s",
					 po->object.rel.dbname, po->object.rel.schema, po->object.rel.relname);
			values[0] = CStringGetDatum(fullname);
			values[1] = CStringGetDatum(b1);
			values[2] = CStringGetDatum(b2);
			tuplestore_putvalues(tupleStore, tupleDesc, values, nulls);

			po = po->next;
		}
	}
	else
	{
		memset(nulls, 0, sizeof(nulls));
		memset(values, 0, sizeof(values));

		values[0] = CStringGetDatum("relation_limit");
		values[1] = CStringGetDatum("null");
		values[2] = CStringGetDatum("null");
		tuplestore_putvalues(tupleStore, tupleDesc, values, nulls);
	}

	tuplestore_donestoring(tupleStore);

	disconnectDatabase(remote_conn);

	return (Datum) 0;
}