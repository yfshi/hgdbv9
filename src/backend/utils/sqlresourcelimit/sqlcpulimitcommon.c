/*-------------------------------------------------------------------------
 *
 * sqlcpulimitcommon.c
 *	  Implenmentation of the cpu limit of sql resource limit functionality
 *
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *
 * IDENTIFICATION
 *	  src/backend/utils/sqlresourcelimit/sqlcpulimitcommon.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#ifndef WIN32
#include <sys/stat.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <mntent.h>
#include "pgstat.h"
#include "utils/guc.h"
#include "utils/sqlresourcelimit.h"

/* Begin - Bug#z8537 */
#ifndef CGROUP2_SUPER_MAGIC
#define CGROUP2_SUPER_MAGIC 0x63677270
#endif
/* End - Bug#z8537 */


#define CURRENT_MOUNTS "/proc/self/mounts"

CGroupBaseInfo *cgroupBaseInfo = NULL;

static
CGroupBaseInfo cgroupSbaseInfo = {
		CGROUP_VARSION_V1,
		""
};

/* get_cgroup_baseinfo
 *
 * Initialize cgroupBaseInfo.
 */
static CGroupBaseInfo *
get_cgroup_baseinfo(void)
{
	return &cgroupSbaseInfo;
}

/*
 * Bug#z8587
 * Check whether the current system use cgroup v2.
 */
static bool
is_cgroup2_controller(void)
{
	struct statfs fs;

	if (statfs("/sys/fs/cgroup", &fs) != 0)
	{
		return false;
	}

	elog(DEBUG5, "f_type = 0x%lux\n", (unsigned long)fs.f_type);

	if (fs.f_type == CGROUP2_SUPER_MAGIC)
	{
		elog(DEBUG5, "cgroup v2\n");
		return true;
	}
	else
	{
		elog(DEBUG5, "not cgroup v2\n");
		return false;
	}
}


/* getCurrentCgroupMountDir
 *
 * Get the mount directory of cgroup, the basic method
 * is to read the file "/proc/self/mounts". Normally,
 * roup version 1 will return "/sys/fs/cgroup/xxx",
 * so we need remove the "xxx", but version 2 do not
 * need this.
 */
bool
getCurrentCgroupMountDir(void)
{
	struct mntent *me;
	FILE *fp;
	bool is_cgroup2 = false; /* Bug#z8587 */

	if (cgroupBaseInfo == NULL)
		cgroupBaseInfo = get_cgroup_baseinfo();

	if (strlen(cgroupBaseInfo->cgroup_dir) != 0)
		return true;

	memset(cgroupBaseInfo->cgroup_dir,'\0',sizeof(cgroupBaseInfo->cgroup_dir));

	fp = setmntent(CURRENT_MOUNTS, "r");
	if (fp == NULL)
		elog(ERROR, "can not open '%s' for read", CURRENT_MOUNTS);

	is_cgroup2 = is_cgroup2_controller(); /* Bug#z8587 */
	while ((me = getmntent(fp)))
	{
		char * p;

		if (strcmp(me->mnt_type, "cgroup2") == 0 && is_cgroup2) /* Bug#z8587 */
		{
			strncpy(cgroupBaseInfo->cgroup_dir, me->mnt_dir, sizeof(cgroupBaseInfo->cgroup_dir));
			cgroupBaseInfo->version = CGROUP_VARSION_V2;
		}
		else if (strcmp(me->mnt_type, "cgroup") == 0 && !is_cgroup2) /* Bug#z8587 */
		{
			/* Because the path returned by cgroup is located
			 * under cgroup/system and is not the root path of
			 * the cgroup, it is necessary to further resolve
			 * and obtain the actual cgroup root path.
			 */
			strncpy(cgroupBaseInfo->cgroup_dir, me->mnt_dir, sizeof(cgroupBaseInfo->cgroup_dir) - 1);
			p = strrchr(cgroupBaseInfo->cgroup_dir, '/');

			if (p == NULL)
				elog(ERROR, "cgroup mount point parse error: %s", cgroupBaseInfo->cgroup_dir);
			else
			{
				char	temcgroup_dir[MAX_CURRENT_CGROUP_PATHLEN];

				/* get the cgroup root path */
				memset(temcgroup_dir, 0, MAX_CURRENT_CGROUP_PATHLEN);
				memcpy(temcgroup_dir, cgroupBaseInfo->cgroup_dir, strlen(cgroupBaseInfo->cgroup_dir) - strlen(p));
				/* copy the path to cgroup_dir */
				memset(cgroupBaseInfo->cgroup_dir, 0, MAX_CURRENT_CGROUP_PATHLEN);
				memcpy(cgroupBaseInfo->cgroup_dir, temcgroup_dir, strlen(temcgroup_dir));
			}
			cgroupBaseInfo->version = CGROUP_VARSION_V1;
		}
		else
		{
			continue;
		}

		break;
	}

	endmntent(fp);

	return strlen(cgroupBaseInfo->cgroup_dir) != 0;
}

/* get_sql_resource_limit_cpu_path
 *
 * Get the cgroup path for CPU limits.
 * In cgroup v1, CPU limits are located in a cpu subdirectory,
 * while in cgroup v2, there is no subdirectory.
 */
char *
get_sql_resource_limit_cpu_path(void)
{
	char path[MAX_CURRENT_CGROUP_PATHLEN];
	struct stat st;

	if (cgroupBaseInfo == NULL && strlen(cgroupBaseInfo->cgroup_dir) == 0)
		return NULL;

	memset(path, 0, sizeof(path));
	if (cgroupBaseInfo->version == CGROUP_VARSION_V1)
	{
		snprintf(path, MAX_CURRENT_CGROUP_PATHLEN, "%s/cpu/sql_resource_limit/",cgroupBaseInfo->cgroup_dir);
	}
	else
	{
		snprintf(path, MAX_CURRENT_CGROUP_PATHLEN, "%s/sql_resource_limit/",cgroupBaseInfo->cgroup_dir);
	}

	if (stat(path, &st) != 0)
	{
		return NULL;
	}

	return pstrdup(path);
}

/* get_sql_resourece_limit_name
 *
 * Get the directory name of the current process's CPU limit.
 */
char *
get_sql_resourece_limit_name(void)
{
	char		path[MAX_CURRENT_CGROUP_PATHLEN];
	char	   *cpupath;

	memset(path, 0, sizeof(path));

	/* get the path of sql_resource_limit */
	cpupath = get_sql_resource_limit_cpu_path();
	if (cpupath == NULL)
	{
		return NULL;
	}

	snprintf(path, MAX_CURRENT_CGROUP_PATHLEN, "%shg_sql_resourece_limit_cpu_%d",cpupath, MyProcPid);
	pfree(cpupath);

	return pstrdup(path);
}

/* create_sql_resource_no_cpu_limit
 *
 * Create a directory without CPU limits.
 */
NoCpuLimit
create_sql_resource_no_cpu_limit(void)
{
	char sql_resource_dir[MAX_CURRENT_CGROUP_PATHLEN];
	char	   *cpupath;
	struct stat st;

	memset(sql_resource_dir, 0, MAX_CURRENT_CGROUP_PATHLEN);

	/*
	 * get the path of sql_resource_limit and
	 * create a directory without CPU limits.
	 */
	cpupath = get_sql_resource_limit_cpu_path();
	if (cpupath == NULL)
	{
		return OLD_NO_CPU_LIMIT_CGROUP;
	}

	snprintf(sql_resource_dir, MAX_CURRENT_CGROUP_PATHLEN, "%s/hg_sql_resourece_not_limit", cpupath);
	pfree(cpupath);

	/*
	 * if the path is aready exits, return
	 * OLD_NO_CPU_LIMIT_CGROUP, otherwise
	 * return NEW_NO_CPU_LIMIT_CGROUP.
	 */
	if (!stat(sql_resource_dir, &st) == 0)
	{
		if (mkdir(sql_resource_dir, 0775) == 0)
		{
			return NEW_NO_CPU_LIMIT_CGROUP;
		}

		elog(ERROR, "create hg_sql_resourece_not_limit failed");
	}

	return OLD_NO_CPU_LIMIT_CGROUP;
}

/* sql_resource_no_cpu_limit
 *
 * Create a directory without CPU limits,
 * and initialize it.
 */
void
sql_resource_no_cpu_limit(void)
{
	NoCpuLimit recv;

	recv = create_sql_resource_no_cpu_limit();

	if (recv == NEW_NO_CPU_LIMIT_CGROUP)
	{
		if (cgroupBaseInfo->version == CGROUP_VARSION_V1)
		{
			init_sql_resource_no_cpu_limit_v1();
		}
		else
		{
			init_sql_resource_no_cpu_limit_v2();
		}
	}
}

/* write_value_to_file
 *
 * write data to the specified file.
 */
void
write_value_to_file(FILE	*fpout, int value)
{
	char	buf[NUM_DATA_LEN];
	int		rc;
	int		len = 0;

	memset(buf, 0, sizeof(buf));
	snprintf(buf, NUM_DATA_LEN, "%d", value);

	len = strlen(buf);

	/* write the value to file */
	rc = fwrite(&buf, len, 1, fpout);
	if (rc != 1)
	{
		FreeFile(fpout);
		ereport(ERROR,
				(errcode(ERRCODE_IO_ERROR),
				 errmsg("Failed to write file.")));
	}
}

/* create_sql_resource_limit_dir_for_current_process
 *
 * Create a cgroup resource grouo for the current process.
 */
bool
create_sql_resource_limit_dir_for_current_process(void)
{
	struct stat st;
	char *sql_resource_dir;

	sql_resource_dir = get_sql_resource_limit_cpu_path();
	if (sql_resource_dir == NULL)
	{
		return false;
	}

	/*
	 * check whether a CPU throttling (CPU limit)
	 * cgroup resource group exists.
	 */
	if (stat(sql_resource_dir, &st) == 0)
	{
		pfree(sql_resource_dir);

		if (S_ISDIR(st.st_mode))
		{
			char *path;

			sql_resource_no_cpu_limit();
			path = get_sql_resourece_limit_name();
			if (path == NULL)
			{
				return false;
			}

			/* if the path is not exits, create it */
			if (stat(path, &st) != 0)
			{
				if (mkdir(path, 0775) == 0)
				{
					pfree(path);
					return true;
				}
			}
			else
			{
				pfree(path);
				return true;
			}
		}
	}

	return false;
}

/* release_sql_resource_limit_cgroup
 *
 * If the disconnected, release the current process,
 * move the process to a non-limited group, and then
 * delete the current process's cgroup.
 */
void
release_sql_resource_limit_cgroup(int current_pid)
{
	char	taskpid[MAX_CURRENT_CGROUP_PATHLEN];
	FILE	*fpout;
	char   *cpugrouppath;
	struct stat st;

	if (cgroupBaseInfo == NULL || source_limit_mode == RESOURCE_LIMIT_OFF)
		return;

	memset(taskpid, 0, sizeof(taskpid));

	cpugrouppath = get_sql_resource_limit_cpu_path();
	if (cpugrouppath == NULL)
	{
		return;
	}

	/* If the cgroup is V1 version, the process
	 * id in tasks, the version is V2, the
	 * process id in cgroup.procs
	 */
	if (cgroupBaseInfo->version == CGROUP_VARSION_V1)
	{
		snprintf(taskpid, MAX_CURRENT_CGROUP_PATHLEN, "%s/hg_sql_resourece_not_limit/tasks", cpugrouppath);
	}
	else
	{
		snprintf(taskpid, MAX_CURRENT_CGROUP_PATHLEN, "%s/hg_sql_resourece_not_limit/cgroup.procs", cpugrouppath);
	}

	if (stat(taskpid, &st) != 0)
	{
		return;
	}

	fpout = AllocateFile(taskpid, PG_BINARY_W);
	if (fpout != NULL)
	{
		write_value_to_file(fpout, current_pid);
		FreeFile(fpout);
	}
	else
	{
		pfree(cpugrouppath);
		ereport(ERROR,
				(errcode(ERRCODE_IO_ERROR),
				 errmsg("Failed to open file %s.", taskpid)));
	}

	/* delete the current process's cgroup
	 * resource control group.
	 */
	{
		char path[MAX_CURRENT_CGROUP_PATHLEN];
		struct stat st;

		memset(path, 0, sizeof(path));
		snprintf(path, MAX_CURRENT_CGROUP_PATHLEN, "%s/hg_sql_resourece_limit_cpu_%d", cpugrouppath, current_pid);
		if (stat(path, &st) == 0)
		{
			if (S_ISDIR(st.st_mode))
			{
				if (rmdir(path) != 0)
					elog(WARNING, "drop sql resource limit file failed");
			}
		}

		pfree(cpugrouppath);
	}
}

/* reset_process_to_origin_cpu
 *
 * If the disconnected, release the current process,
 * move the process to a non-limited group, and then
 * delete the current process's cgroup.
 */
void
reset_process_to_origin_cpu(void)
{
	char	taskfile[MAX_CURRENT_CGROUP_PATHLEN];
	FILE	*fpout;
	struct stat st;

	if (source_limit_mode == RESOURCE_LIMIT_OFF)
		return;

	memset(taskfile, 0, sizeof(taskfile));

	/* If cup_prev_in_cgroup is not null, need to write
	 * the pid to origin path, otherwise write the pid
	 * to none cpu limit cgroup.
	 */
	if (cup_prev_in_cgroup != NULL && strcmp(cup_prev_in_cgroup, "") != 0)
	{
		snprintf(taskfile, MAX_CURRENT_CGROUP_PATHLEN, "%s", cup_prev_in_cgroup);
	}
	else
	{
		if (cgroupBaseInfo == NULL)
			return;

		if (cgroupBaseInfo->version == CGROUP_VARSION_V1)
		{
			snprintf(taskfile, MAX_CURRENT_CGROUP_PATHLEN, "%s/cpu/sql_resource_limit/hg_sql_resourece_not_limit/tasks", cgroupBaseInfo->cgroup_dir);
		}
		else
		{
			snprintf(taskfile, MAX_CURRENT_CGROUP_PATHLEN, "%s/sql_resource_limit/hg_sql_resourece_not_limit/cgroup.procs", cgroupBaseInfo->cgroup_dir);
		}
	}

	if (stat(taskfile, &st) != 0)
	{
		return;
	}

	fpout = AllocateFile(taskfile, PG_BINARY_W);
	if (fpout != NULL)
	{
		write_value_to_file(fpout, MyProcPid);
		FreeFile(fpout);
	}
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_IO_ERROR),
				 errmsg("Failed to open file %s.", taskfile)));
	}

	if (cup_prev_in_cgroup)
	{
		pfree(cup_prev_in_cgroup);
	}

	cup_prev_in_cgroup = NULL;
}

/* get_current_process_origen_cpu_path_incgroup
 *
 * Get which cgroup the current process belongs to.
 */
void
get_current_process_origen_cpu_path_incgroup(void)
{
	char	   *cpupath = NULL;
	char		filepath[MAX_CURRENT_CGROUP_PATHLEN];
	char		fileobjpath[MAX_CURRENT_CGROUP_PATHLEN];

	if (source_limit_mode == RESOURCE_LIMIT_OFF)
		return;

	memset(filepath, 0, sizeof(filepath));
	memset(fileobjpath, 0, sizeof(fileobjpath));

	/* Get which cgroup the current process belongs to */
	cpupath = sql_resource_limit_get_process_origin_path();
	if (cpupath != NULL)
	{
		if (strncmp(cpupath, "/sql_resource_limit", strlen("/sql_resource_limit")) &&
			strncmp(cpupath, "/highgo", strlen("/highgo")) == 0)
		{
			if (cgroupBaseInfo->version == CGROUP_VARSION_V1)
			{
				snprintf(filepath, MAX_CURRENT_CGROUP_PATHLEN, "%s/cpu%s", cgroupBaseInfo->cgroup_dir, cpupath);
			}
			else
			{
				snprintf(filepath, MAX_CURRENT_CGROUP_PATHLEN, "%s%s", cgroupBaseInfo->cgroup_dir, cpupath);
			}
		}
		else
		{
			pfree(cpupath);
			return;
		}
	}
	else
		return;

	if (strlen(filepath) == 0)
		return;

	/* The filename correspoding to the process. */
	if (cgroupBaseInfo->version == CGROUP_VARSION_V1)
		snprintf(fileobjpath, MAX_CURRENT_CGROUP_PATHLEN, "%s/tasks", filepath);
	else
		snprintf(fileobjpath, MAX_CURRENT_CGROUP_PATHLEN, "%s/cgroup.procs", filepath);

	pfree(cpupath);

	cup_prev_in_cgroup = pstrdup(fileobjpath);
}

/* sql_resource_limit_get_process_origin_path
 *
 * Get which cgroup the current process belongs to.
 */
char *
sql_resource_limit_get_process_origin_path(void)
{
	char	*pathname = NULL;
	FILE	*fp = fopen("/proc/self/cgroup", "r");

	if (fp)
	{
		char line[MAX_CURRENT_CGROUP_PATHLEN];

		while (fgets(line, sizeof(line), fp))
		{
			int		id;
			char	controllers[MAX_PATHDATA_LEN];
			char	path[MAX_CURRENT_CGROUP_PATHLEN];

			memset(controllers, 0, sizeof(controllers));
			memset(path, 0, sizeof(path));

			if (cgroupBaseInfo->version == CGROUP_VARSION_V1)
			{
				/* when the cgroup is version v1, the value
				 * such as 5:cpuacct,cpu:/path
				 */
				if (sscanf(line, "%d:%127[^:]:%255s", &id, controllers, path) == 3)
				{
					if (strlen(controllers) > 0 && strcmp(pstrdup(controllers), "cpuset") &&
						strstr(controllers, "cpu") && (path[0] == '/'))
					{
						pathname = pstrdup(path);
						break;
					}
				}
			}
			else
			{
				/* when the cgroup is version v2, the value is
				 * 0::/path
				 */
				char	   *p = strchr(line, ':');

				if (p != NULL && strlen(p) > 2)
				{
					p = strchr(p+1, ':');

					if (p != NULL)
					{
						int		plen = 0;

						/* the p include a trailing '\n', Since it is as a
						 * filesystem path, the '\n' needs to be removed.
						 */
						strcpy(path, p+1);
						if ((plen = strlen(path)) > 0)
						{
							if (path[plen-1] == '\n')
							{
								char temppath[MAX_CURRENT_CGROUP_PATHLEN];

								memset(temppath, 0, sizeof(temppath));
								strncpy(temppath, path, plen-1);
								pathname = pstrdup(temppath);
							}
							else
							{
								pathname = pstrdup(path);
							}
							break;
						}
					}
				}
			}
		}

		fclose(fp);
	}

	return pathname;
}

/* drop_all_sql_resoure_limit_for_cpu_handle
 *
 * when the database startsm, delete all CPU-limited cgroup.
 */
void
drop_all_sql_resoure_limit_for_cpu_handle(void)
{
	char	   *path;
	struct stat st;

	path = get_sql_resource_limit_cpu_path();

	if (path == NULL)
		return;

	if (stat(path, &st) == 0)
	{
		DIR		   *dir;

		if (!S_ISDIR(st.st_mode))
		{
			pfree(path);
			return;
		}

		if ((dir = opendir(path)) == NULL)
		{
			pfree(path);
			return;
		}
		else
		{
			struct dirent *entry;
			char filename[MAX_CURRENT_CGROUP_PATHLEN];

			/*
			 * Open the sql_resource_limit directory and
			 * delete all resource control group.
			 */
			while ((entry = readdir(dir)) != NULL )
			{
				memset(filename, 0, sizeof(filename));
				snprintf(filename, MAX_CURRENT_CGROUP_PATHLEN, "%s%s", pstrdup(path), entry->d_name);

				if (stat(filename, &st) == 0)
				{
					if (S_ISDIR(st.st_mode))
					{
						/* If the filder name starts with 'hg_sql_resourece',
						 * delete it entirely.
						 */
						if (strncmp(entry->d_name, "hg_sql_resourece", strlen("hg_sql_resourece")))
							continue;

						if (rmdir(filename) != 0)
							elog(WARNING, "drop sql resource limit file failed");
					}
				}
			}

			closedir(dir);
		}
	}

	pfree(path);
}
#endif
