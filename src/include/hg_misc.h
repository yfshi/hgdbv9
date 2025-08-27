#ifndef HG_MISC_H
#define HG_MISC_H

#include "postgres.h"

#include "access/genam.h"
#include "access/heapam.h"
#include "storage/lwlock.h"

#ifdef MISC_INTERNAL /* Begin - ReqID:SRS-Performance-GUC */
extern bool *skip_log;
/* Modes for processes to GUC special index oid */
typedef enum
{
	PDBG_AUTOSET_NORMAL = 0,
	PDBG_AUTOSET_INIT,
	PDBG_AUTOSET_PSQL,
	PDBG_AUTOSET_RESET_ALL
} AutosetMode;

typedef void (*snapshot_lock_acquire_hook_type)(LWLockMode mode);
extern PGDLLIMPORT snapshot_lock_acquire_hook_type snapshot_lock_acquire_hook;
typedef void (*snapshot_lock_release_hook_type)(void);
extern PGDLLIMPORT snapshot_lock_release_hook_type snapshot_lock_release_hook;

typedef void (*haep_page_prune_hook_type)(Relation relation, Buffer buffer,
                                          struct GlobalVisState *vistest, 
                                          TransactionId limited_xmin,
                                          TimestampTz limited_ts);
extern PGDLLIMPORT haep_page_prune_hook_type haep_page_prune_hook;

typedef bool (*skip_special_indexoid_hook_type)(IndexScanDesc scan);
extern PGDLLIMPORT skip_special_indexoid_hook_type skip_special_indexoid_hook;

typedef Size (*clog_shmem_buffers_hook_type)(void);
extern PGDLLIMPORT clog_shmem_buffers_hook_type clog_shmem_buffers_hook;

typedef bool (*pdbg_log_hook_type)(void);
extern PGDLLIMPORT pdbg_log_hook_type pdbg_log_hook;

typedef void (*pdbg_autoset_hook_type)(AutosetMode mode);
extern PGDLLIMPORT pdbg_autoset_hook_type pdbg_autoset_hook;
#endif /* ReqID:SRS-Performance-GUC */

#endif /* HG_MISC_H */