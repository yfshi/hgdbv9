/*-------------------------------------------------------------------------
 *
 * sqlcpulimit_v1.c
 *	  Implenmentation of the cpu limit of sql resource limit functionality
 *
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *
 * IDENTIFICATION
 *	  src/backend/utils/sqlresourcelimit/sqlcpulimit_v1.c
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

/* init_sql_resource_no_cpu_limit_v1
 *
 * Initialize a CPU-unlimited cgroup from cgroup v1.
 */
void
init_sql_resource_no_cpu_limit_v1(void)
{
	char		cfs_period_us[MAX_CURRENT_CGROUP_PATHLEN];
	char		cfs_quota_us[MAX_CURRENT_CGROUP_PATHLEN];
	FILE	   *fpout;
	char		path[MAX_CURRENT_CGROUP_PATHLEN];
	char	   *cpupath;
	struct stat st;

	memset(cfs_period_us, 0, sizeof(cfs_period_us));
	memset(cfs_quota_us, 0, sizeof(cfs_quota_us));
	memset(path, 0, sizeof(path));

	cpupath = get_sql_resource_limit_cpu_path();
	if (cpupath == NULL)
	{
		return;
	}

	snprintf(path, MAX_CURRENT_CGROUP_PATHLEN, "%shg_sql_resourece_not_limit", cpupath);
	pfree(cpupath);

	/*
	 * Set the cpu.cfs_period_us maximum.
	 */
	snprintf(cfs_period_us, MAX_CURRENT_CGROUP_PATHLEN, "%s/cpu.cfs_period_us", path);
	if (stat(cfs_period_us, &st) != 0)
	{
		return;
	}

	fpout = AllocateFile(cfs_period_us, PG_BINARY_W);
	if (fpout != NULL)
	{
		write_value_to_file(fpout, 1000000);
		FreeFile(fpout);
	}
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_IO_ERROR),
				 errmsg("Failed to open file %s.", cfs_period_us)));
	}

	/*
	 * Disable CPU throttling.
	 */
	snprintf(cfs_quota_us, MAX_CURRENT_CGROUP_PATHLEN, "%s/cpu.cfs_quota_us", path);
	if (stat(cfs_quota_us, &st) != 0)
	{
		return;
	}

	fpout = AllocateFile(cfs_quota_us, PG_BINARY_W);
	if (fpout != NULL)
	{
		write_value_to_file(fpout, -1);
		FreeFile(fpout);
	}
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_IO_ERROR),
				 errmsg("Failed to open file %s.", cfs_quota_us)));
	}
}


/* set_current_process_cpu_rate_v1
 *
 * Limite the cpu rate of the current process for cgrop v1.
 */
void
set_current_process_cpu_rate_v1(void)
{
	char	   *path;
	char		taskpid[MAX_CURRENT_CGROUP_PATHLEN];
	char		cfs_period_us[MAX_CURRENT_CGROUP_PATHLEN];
	char		cfs_quota_us[MAX_CURRENT_CGROUP_PATHLEN];
	FILE	   *fpout;
	struct stat st;

	memset(taskpid, 0, sizeof(taskpid));
	memset(cfs_period_us, 0, sizeof(cfs_period_us));
	memset(cfs_quota_us, 0, sizeof(cfs_quota_us));

	path = get_sql_resourece_limit_name();
	if (path == NULL)
	{
		return;
	}

	/*
	 * Add the current process to its
	 * corresponding CPU throttling
	 * (rate-limited) cgroup.
	 */
	snprintf(taskpid, MAX_CURRENT_CGROUP_PATHLEN, "%s/tasks", path);
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

	/*
	 * Set the cpu.cfs_period_us maximum.
	 */
	snprintf(cfs_period_us, MAX_CURRENT_CGROUP_PATHLEN, "%s/cpu.cfs_period_us", path);
	if (stat(cfs_period_us, &st) != 0)
	{
		return;
	}

	fpout = AllocateFile(cfs_period_us, PG_BINARY_W);
	if (fpout != NULL)
	{
		write_value_to_file(fpout, 1000000);
		FreeFile(fpout);
	}
	else
	{
		pfree(path);
		ereport(ERROR,
				(errcode(ERRCODE_IO_ERROR),
				 errmsg("Failed to open file %s.", cfs_period_us)));
	}

	/*
	 * Set the maximum CPU usage percentage.
	 */
	snprintf(cfs_quota_us, MAX_CURRENT_CGROUP_PATHLEN, "%s/cpu.cfs_quota_us", path);
	if (stat(cfs_quota_us, &st) != 0)
	{
		return;
	}

	fpout = AllocateFile(cfs_quota_us, PG_BINARY_W);
	if (fpout != NULL)
	{
		write_value_to_file(fpout, 10000 * sqlcpu_usage);
		FreeFile(fpout);
	}
	else
	{
		pfree(path);
		ereport(ERROR,
				(errcode(ERRCODE_IO_ERROR),
				 errmsg("Failed to open file %s.", cfs_quota_us)));
	}

	pfree(path);
}
#endif
