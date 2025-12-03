/* Begin ReqID: SRS-SESSION-VIEWS */
/*-------------------------------------------------------------------------
 *
 * hg_proc_data.h
 *	  Functions for accessing the session statistic data
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src\backend\utils\adt\hg_proc_data.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef HG_PROC_DATA_H
#define HG_PROC_DATA_H

#include "funcapi.h"
#include "miscadmin.h"
#include "libpq/libpq-be.h"


#define PROC_INFO_STR_LEN 32
#define PATH_SEARCH_LEN (PROC_INFO_STR_LEN*8)

extern int64 g_proc_counter;
extern bool count_idle_busy_time;

/* entry for buffer lookup hashtable */
struct proc_data
{
	unsigned int	sid;
	char			busy_num[PROC_INFO_STR_LEN];
	char			idle_sum[PROC_INFO_STR_LEN];
	char			trans_info[PROC_INFO_STR_LEN];
	char			path_search[PATH_SEARCH_LEN];
	unsigned int	tid;
	bool			is_lock;
	bool			is_share;
};

/* entry for buffer lookup hashtable */
typedef struct
{
	int					key;
	struct proc_data	data;
} ProcPrivEnt;


extern void ProcDataShmemInit(void); 
extern Size ProcDataShmemSize(void);
extern void ProcDataTableInsert(const int *tagPtr, int sid, int trans_state, unsigned int tid);
unsigned int ProcDataTableLookup(const int *tagPtr, u_int32_t *sid, char *idle_sum,
	char *busy_num, char *trans_info, u_int32_t *tid, char* path_search, bool *islock, bool *is_share);
void ProcDataDelete(const int *tagPtr);
void RegisterOnExithandler(void);
void ClearProcDataOnExit(int code, Datum arg);
/* End ReqID: SRS-SESSION-VIEWS */

#endif
