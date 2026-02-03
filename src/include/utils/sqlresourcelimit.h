/*-------------------------------------------------------------------------
 *
 * sqlresourcelimit.h
 *	  Implenmentation of the sql resource limit functionality
 *
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *
 * IDENTIFICATION
 *	  src/include/utils/sqlresourcelimit.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef SQLRESOURCELIMIT_H
#define SQLRESOURCELIMIT_H

#include "utils/guc.h"
#include "utils/resource_manager.h"

#define MAX_CURRENT_CGROUP_PATHLEN 256
#define MAX_PATHDATA_LEN 128
#define NUM_DATA_LEN 32

typedef enum
{
	CGROUP_VARSION_V1,
	CGROUP_VARSION_V2,
}CgroupVerison;

typedef struct CGroupBaseInfo
{
	/* The cgroup version on this machine */
	CgroupVerison version;

	/* The cgroup mount dir */
	char cgroup_dir[MAX_CURRENT_CGROUP_PATHLEN];
} CGroupBaseInfo;

typedef enum
{
	NEW_NO_CPU_LIMIT_CGROUP,
	OLD_NO_CPU_LIMIT_CGROUP,
}NoCpuLimit;

typedef enum
{
	MEM_ADD,
	MEM_SUB
}MemCountOpt;

extern long long int sqlallocated_memorycount;
extern long long int sqlread_buffercount;
extern long long int sqlwrite_buffercount;
extern char *cup_prev_in_cgroup;
extern CGroupBaseInfo *cgroupBaseInfo;

#ifndef WIN32
/* sqlcpulimit_v1.c */
extern void init_sql_resource_no_cpu_limit_v1(void);
extern void set_current_process_cpu_rate_v1(void);

/* sqlcpulimit_v2.c */
extern void init_sql_resource_no_cpu_limit_v2(void);
extern void set_current_process_cpu_rate_v2(void);

/* sqlcpulimitcommon.c */
extern bool getCurrentCgroupMountDir(void);
extern char *get_sql_resource_limit_cpu_path(void);
extern char *get_sql_resourece_limit_name(void);
extern NoCpuLimit create_sql_resource_no_cpu_limit(void);
extern void sql_resource_no_cpu_limit(void);
extern void write_value_to_file(FILE	*fpout, int value);
extern bool create_sql_resource_limit_dir_for_current_process(void);
extern void release_sql_resource_limit_cgroup(int current_pid);
extern void reset_process_to_origin_cpu(void);
extern void get_current_process_origen_cpu_path_incgroup(void);
extern char *sql_resource_limit_get_process_origin_path(void);
extern void drop_all_sql_resoure_limit_for_cpu_handle(void);
#endif

/* sqlresourcelimit.c */
extern void sql_resource_limit_check_io_read(void);
extern void sql_resource_limit_check_io_write_count(void);
extern void sql_resource_limit_check_io_write(void);
extern void sql_resource_limit_check_memory(Size size, MemCountOpt opt);
extern void sql_resource_limit_start(void);
#ifndef WIN32
extern void drop_all_sql_resoure_limit_for_cpu(void);
#endif

#endif
