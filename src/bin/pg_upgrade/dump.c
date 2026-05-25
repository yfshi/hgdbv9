/*
 *	dump.c
 *
 *	dump functions
 *
 *	Copyright (c) 2010-2021, PostgreSQL Global Development Group
 *	src/bin/pg_upgrade/dump.c
 */

#include "postgres_fe.h"

#include "fe_utils/string_utils.h"
#include "pg_upgrade.h"

void
generate_old_dump(void)
{
	int			dbnum;

	/* Begin - BUG #Z7100 */
	PQExpBufferData exclude_ext;

	if (old_cluster.extension_exclude_patterns.head != NULL)
	{
		SimpleStringListCell *cell;
		initPQExpBuffer(&exclude_ext);
		for (cell = old_cluster.extension_exclude_patterns.head; cell; cell = cell->next)
		{
			appendPQExpBuffer(&exclude_ext, "--exclude-extension %s ", cell->val);
		}
	}
	/* End - BUG #Z7100 */

	prep_status("Creating dump of global objects");

	/* run new pg_dumpall binary for globals */
/* BEGIN - ReqID:SRS-CMD-highgo2pg */
#ifdef HG2PG
	exec_prog(UTILITY_LOG_FILE, NULL, true, true,
			  "\"%s/pg_dumpall\" %s%s --globals-only --quote-all-identifiers "
			  "--binary-upgrade %s -f %s",
			  old_cluster.bindir, cluster_conn_opts(&old_cluster),
			  protocol_negotiation_supported(&old_cluster) ? "" : " -d \"max_protocol_version=3.0\"",
			  log_opts.verbose ? "--verbose" : "",
			  GLOBALS_DUMP_FILE);
#else
/* END - ReqID:SRS-CMD-highgo2pg */
	exec_prog(UTILITY_LOG_FILE, NULL, true, true,
			  "\"%s/pg_dumpall\" %s%s --globals-only --quote-all-identifiers "
			  "--binary-upgrade %s -f %s %s",		// BUG #Z7100
			  new_cluster.bindir, cluster_conn_opts(&old_cluster),
			  protocol_negotiation_supported(&old_cluster) ? "" : " -d \"max_protocol_version=3.0\"",
			  log_opts.verbose ? "--verbose" : "",
			  GLOBALS_DUMP_FILE,
			  old_cluster.extension_exclude_patterns.head != NULL ? exclude_ext.data : ""	// BUG #Z7100
			  );
#endif /* ReqID:SRS-CMD-highgo2pg */
	check_ok();

	prep_status("Creating dump of database schemas\n");

	/* create per-db dump files */
	for (dbnum = 0; dbnum < old_cluster.dbarr.ndbs; dbnum++)
	{
		char		sql_file_name[MAXPGPATH],
					log_file_name[MAXPGPATH];
		DbInfo	   *old_db = &old_cluster.dbarr.dbs[dbnum];
		PQExpBufferData connstr,
					escaped_connstr;

		initPQExpBuffer(&connstr);
		appendPQExpBufferStr(&connstr, "dbname=");
		appendConnStrVal(&connstr, old_db->db_name);
		if (!protocol_negotiation_supported(&old_cluster))
			appendPQExpBufferStr(&connstr, " max_protocol_version=3.0");

		initPQExpBuffer(&escaped_connstr);
		appendShellString(&escaped_connstr, connstr.data);
		termPQExpBuffer(&connstr);

		pg_log(PG_STATUS, "%s", old_db->db_name);
		snprintf(sql_file_name, sizeof(sql_file_name), DB_DUMP_FILE_MASK, old_db->db_oid);
		snprintf(log_file_name, sizeof(log_file_name), DB_DUMP_LOG_FILE_MASK, old_db->db_oid);

/* BEGIN - ReqID:SRS-CMD-highgo2pg */
#ifdef HG2PG
		parallel_exec_prog(log_file_name, NULL,
						   "\"%s/pg_dump\" %s --schema-only --quote-all-identifiers "
						   "--binary-upgrade --format=custom %s --file=\"%s\" %s",
						   old_cluster.bindir, cluster_conn_opts(&old_cluster),
						   log_opts.verbose ? "--verbose" : "",
						   sql_file_name, escaped_connstr.data);
#else
/* END - ReqID:SRS-CMD-highgo2pg */
		parallel_exec_prog(log_file_name, NULL,
						   "\"%s/pg_dump\" %s --schema-only --quote-all-identifiers "		// BUG #Z7100
						   "--binary-upgrade --format=custom %s --file=\"%s\" %s %s",
						   new_cluster.bindir, cluster_conn_opts(&old_cluster),
						   log_opts.verbose ? "--verbose" : "",
						   sql_file_name, escaped_connstr.data,
						   old_cluster.extension_exclude_patterns.head != NULL ? exclude_ext.data : ""	// BUG #Z7100
						   );
#endif /* ReqID:SRS-CMD-highgo2pg */

		termPQExpBuffer(&escaped_connstr);
	}

	/* Begin - BUG #Z7100 */
	if (old_cluster.extension_exclude_patterns.head != NULL)
		termPQExpBuffer(&exclude_ext);
	/* End - BUG #Z7100 */

	/* reap all children */
	while (reap_child(true) == true)
		;

	end_progress_output();
	check_ok();
}
