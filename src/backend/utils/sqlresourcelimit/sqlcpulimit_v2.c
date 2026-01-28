/*-------------------------------------------------------------------------
 *
 * sqlcpulimit_v2.c
 *	  Implenmentation of the cpu limit of sql resource limit functionality
 *
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *
 * IDENTIFICATION
 *	  src/backend/utils/sqlresourcelimit/sqlcpulimit_v2.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#ifndef WIN32
#include <sys/stat.h>
#include <unistd.h>
#include <mntent.h>
#include "pgstat.h"
#include "utils/guc.h"
#include "utils/sqlresourcelimit.h"

static void write_value_to_file_str_v2(FILE	*fpout, char *value);

/* write_value_to_file_str_v2
 *
 * write data to the specified file..
 */
static void
write_value_to_file_str_v2(FILE	*fpout, char *value)
{
	int		rc;
	int		len = 0;

	len = strlen(value);

	/* write the value to file */
	rc = fwrite(value, len, 1, fpout);
	if (rc != 1)
	{
		FreeFile(fpout);
		ereport(ERROR,
				(errcode(ERRCODE_IO_ERROR),
				 errmsg("Failed to write file.")));
	}
}

/* init_sql_resource_no_cpu_limit_v2
 *
 * Initialize a CPU-unlimited cgroup from cgroup v2.
 */
void
init_sql_resource_no_cpu_limit_v2(void)
{
	char		cpumax[MAX_CURRENT_CGROUP_PATHLEN];
	FILE	   *fpout;
	char		path[MAX_CURRENT_CGROUP_PATHLEN];
	char	   *cpupath;

	memset(cpumax, 0, sizeof(cpumax));
	memset(path, 0, sizeof(path));

	cpupath = get_sql_resource_limit_cpu_path();
	if (cpupath == NULL)
	{
		return;
	}

	snprintf(path, MAX_CURRENT_CGROUP_PATHLEN, "%shg_sql_resourece_not_limit", cpupath);
	pfree(cpupath);

	/*
	 * Set the cpu.max to not limit.
	 */
	snprintf(cpumax, MAX_CURRENT_CGROUP_PATHLEN, "%s/cpu.max", path);
	fpout = AllocateFile(cpumax, PG_BINARY_W);
	if (fpout != NULL)
	{
		write_value_to_file_str_v2(fpout, "max 100000");
		FreeFile(fpout);
	}
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_IO_ERROR),
				 errmsg("Failed to open file %s.", cpumax)));
	}
}

/* set_current_process_cpu_rate_v2
 *
 * Limite the cpu rate of the current process for cgrop v2.
 */
void
set_current_process_cpu_rate_v2(void)
{
	char	   *path;
	char		taskpid[MAX_CURRENT_CGROUP_PATHLEN];
	char		cpumax[MAX_CURRENT_CGROUP_PATHLEN];
	FILE	   *fpout;
	struct stat st;

	memset(taskpid, 0, sizeof(taskpid));
	memset(cpumax, 0, sizeof(cpumax));

	path = get_sql_resourece_limit_name();
	if (path == NULL)
	{
		return;
	}

	/*
	 * Set the cpu.cfs_period_us maximum.
	 */
	snprintf(cpumax, MAX_CURRENT_CGROUP_PATHLEN, "%s/cpu.max", path);
	if (stat(cpumax, &st) != 0)
	{
		return;
	}

	fpout = AllocateFile(cpumax, PG_BINARY_W);
	if (fpout != NULL)
	{
		char buf[NUM_DATA_LEN];

		memset(buf, 0, sizeof(buf));
		snprintf(buf, NUM_DATA_LEN, "%d 100000", 1000 * sqlcpu_usage);

		write_value_to_file_str_v2(fpout, pstrdup(buf));
		FreeFile(fpout);
	}
	else
	{
		pfree(path);
		ereport(ERROR,
				(errcode(ERRCODE_IO_ERROR),
				 errmsg("Failed to open file %s.", cpumax)));
	}

	/*
	 * Add the current process to its
	 * corresponding CPU throttling
	 * (rate-limited) cgroup.
	 */
	snprintf(taskpid, MAX_CURRENT_CGROUP_PATHLEN, "%s/cgroup.procs", path);
	if (stat(taskpid, &st) != 0)
	{
		return;
	}

	fpout = AllocateFile(taskpid, PG_BINARY_W);
	if (fpout != NULL)
	{
		write_value_to_file(fpout, MyProcPid);
		FreeFile(fpout);
	}
	else
	{
		pfree(path);
		ereport(ERROR,
				(errcode(ERRCODE_IO_ERROR),
				 errmsg("Failed to open file %s.", taskpid)));
	}

	pfree(path);
}
#endif
