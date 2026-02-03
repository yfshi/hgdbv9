/*-------------------------------------------------------------------------
 *
 * sqlresourcelimit.c
 *	  Implenmentation of the sql resource limit functionality
 *
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *
 * IDENTIFICATION
 *	  src/backend/utils/sqlresourcelimit/sqlresourcelimit.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#include "miscadmin.h"
#include "utils/guc.h"
#include "utils/sqlresourcelimit.h"
#include "utils/resgroup.h"
#include "storage/proc.h"

#define SQLRESOURCELIMITIO_I sqlread_buffer * 1024LL * 1024
#define SQLRESOURCELIMITIO_O sqlwrite_buffer * 1024LL * 1024
#define SQLRESOURCELIMITMEMORY sqlallocated_memory * 1024LL * 1024

long long int sqlallocated_memorycount = 0;	/* record allocated memory size */
long long int sqlread_buffercount = 0;
long long int sqlwrite_buffercount = 0;	/* there is I of IO */
bool sqlallocatedmem_warning = false;
bool sqlIO_I_warning = false;
bool sqlIO_O_warning = false;
char *cup_prev_in_cgroup = NULL;

static void sql_resource_limit_init_variables();
static void sql_resource_limit_handle_cpu(void);

/* sql_resource_limit_init_variables
 *
 * Initialize SQL resource limit variable.
 */
static void
sql_resource_limit_init_variables()
{
	sqlallocated_memorycount = 0;
	sqlread_buffercount = 0;
	sqlwrite_buffercount = 0;
	sqlallocatedmem_warning = false;
	sqlIO_I_warning = false;
	sqlIO_O_warning = false;
	cup_prev_in_cgroup = NULL;
}

/* sql_resource_limit_check_io_read
 *
 * SQL resource limit for IO operations.
 */
void
sql_resource_limit_check_io_read(void)
{
	int		elevel;

	if (source_limit_mode == RESOURCE_LIMIT_OFF)
		return;

	elevel = (source_limit_mode == RESOURCE_LIMIT_STRICT) ? ERROR : WARNING;

	/* Read one 8K block at a time */
	sqlread_buffercount = sqlread_buffercount + BLCKSZ;

	if (SQLRESOURCELIMITIO_I < sqlread_buffercount && !sqlIO_O_warning)
	{
		sqlIO_O_warning = true;
		ereport(elevel,
			(errcode(ERRCODE_INVALID_LIMIT_VALUE),
			 errmsg("The SQL resource limit has exceeded the maximum allowed IO input value, please check sqlread_buffer")));
	}
}

/* sql_resource_limit_check_io_write_count
 *
 * SQL resource limit for IO operations.
 */
void
sql_resource_limit_check_io_write_count(void)
{
	if (source_limit_mode == RESOURCE_LIMIT_OFF)
		return;

	/* write one 8K block at a time */
	sqlwrite_buffercount = sqlwrite_buffercount + BLCKSZ;
}

void
sql_resource_limit_check_io_write()
{
	int 	elevel;

	if (source_limit_mode == RESOURCE_LIMIT_OFF)
		return;

	elevel = (source_limit_mode == RESOURCE_LIMIT_STRICT) ? ERROR : WARNING;

	if (SQLRESOURCELIMITIO_O < sqlwrite_buffercount && !sqlIO_I_warning)
	{
		sqlIO_I_warning = true;
		ereport(elevel,
			(errcode(ERRCODE_INVALID_LIMIT_VALUE),
			 errmsg("The SQL resource limit has exceeded the maximum allowed IO output value, please check sqlwrite_buffer")));
	}

}

/* sql_resource_limit_check_memory
 *
 * SQL resource limit for memory limit.
 */
void
sql_resource_limit_check_memory(Size size, MemCountOpt opt)
{
	int		elevel;
	long long int newsize = 0;

	if (source_limit_mode != RESOURCE_LIMIT_OFF)
	{
		if (opt == MEM_SUB)
		{
			newsize -= (long long int)size;
		}
		else
		{
			newsize = (long long int)size;
		}

		if ((sqlallocated_memorycount = sqlallocated_memorycount + newsize) < 0)
		{
			sqlallocated_memorycount = 0;

			return;
		}

		/* No error to be reported for memory release */
		if (newsize > 0 && SQLRESOURCELIMITMEMORY < sqlallocated_memorycount &&
			!sqlallocatedmem_warning)
		{
			sqlallocatedmem_warning = true;
			elevel = (source_limit_mode == RESOURCE_LIMIT_STRICT) ? ERROR : WARNING;
			ereport(elevel,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("The SQL resource limit has exceeded the maximum allowed memory value, please check sqlallocated_memory")));
		}
	}

	/* Begin - ReqID:SRS-SECURITY-RESOURCEMANAGEMENT */
	/* User memory resources are restricted only for backend processes. */
	if (MyBackendType == B_BACKEND &&
		MyProc != NULL &&
		IsResGroupEnabled())
	{
		long long int group_memory;

		if (opt == MEM_SUB)
		{
			newsize -= (long long int)size;
		}
		else
		{
			newsize = (long long int)size;
		}

		if ((sqlallocated_memorycount = sqlallocated_memorycount + newsize) < 0)
		{
			sqlallocated_memorycount = 0;
			return;
		}

		group_memory = get_group_memory_limit();

		/*
		 * If the current memory usage exceeds the memory limit configured
		 * for the user's resource group, an error is reported and the
		 * operation is aborted.
		 */
		if (newsize > 0 &&
			!sqlallocatedmem_warning &&
			group_memory > 0 &&
			group_memory < sqlallocated_memorycount)
		{
			sqlallocatedmem_warning = true;
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("SQL resource limit-maxmum memory allocation is \"%lld\", now memory is \"%lld\"",
									group_memory, sqlallocated_memorycount)));
		}
	}
	/* End - ReqID:SRS-SECURITY-RESOURCEMANAGEMENT */
}

#ifndef WIN32
/* sql_resource_limit_handle_cpu
 *
 * SQL resource limit for cpu handle.
 */
static void
sql_resource_limit_handle_cpu(void)
{
	/*
	 * No cgroup resource groups were
	 * found in the current system, so
	 * return directly.
	 */
	if (!getCurrentCgroupMountDir())
	{
		return;
	}

	/* Record which CPU cgroup the current process belong to. */
	get_current_process_origen_cpu_path_incgroup();

	if (cgroupBaseInfo->version == CGROUP_VARSION_V1)
	{
		/*
		 * If the cgroup does not exist, create it
		 * first, if it already exists, write the
		 * data directly.
		 */
		if (create_sql_resource_limit_dir_for_current_process())
		{
			set_current_process_cpu_rate_v1();
		}
	}
	else if (cgroupBaseInfo->version == CGROUP_VARSION_V2)
	{
		/*
		 * If the cgroup does not exist, create it
		 * first, if it already exists, write the
		 * data directly.
		 */
		if (create_sql_resource_limit_dir_for_current_process())
		{
			set_current_process_cpu_rate_v2();
		}
	}
	else
	{
		/* do nothing */
	}
}

/* drop_all_sql_resoure_limit_for_cpu
 *
 * SQL resource limit for cpu, drop all cgroup.
 */
void
drop_all_sql_resoure_limit_for_cpu(void)
{
	if (getCurrentCgroupMountDir())
	{
		drop_all_sql_resoure_limit_for_cpu_handle();
	}
}
#endif

/* sql_resource_limit_start
 *
 * SQL resource limit start.
 */
void
sql_resource_limit_start(void)
{
	/* Initialize SQL resource limit variables. */
	sql_resource_limit_init_variables();

	/* Begin - ReqID:SRS-SECURITY-RESOURCEMANAGEMENT */
	if (source_limit_mode == RESOURCE_LIMIT_OFF)
		return;
	/* End - ReqID:SRS-SECURITY-RESOURCEMANAGEMENT */

#ifndef WIN32
	/* CPU throttling handling. */
	sql_resource_limit_handle_cpu();
#endif
}
