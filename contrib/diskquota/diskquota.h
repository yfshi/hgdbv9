#ifndef DISK_QUOTA_H
#define DISK_QUOTA_H

#include "storage/lwlock.h"
#include "storage/latch.h"

/* timeout count to wait response from launcher process, in 1/10 sec */
#define WAIT_TIME_COUNT  120
#define QUOTA_MAX_DB_NUM 1024

typedef enum
{
	NAMESPACE_QUOTA,
	ROLE_QUOTA
} QuotaType;

typedef enum QuotaSwitch
{
	AUOTA_INVALID = -1,
	AUOTA_OFF = 0,		/* disabled */
	AUOTA_ON		/* enabled while server is running normally */
} quotaSwitch;

struct DiskQuotaLocks
{
	LWLock *active_table_lock;
	LWLock *black_map_lock;
	LWLock *message_box_lock;
	LWLock *quota_state_lock;
};
typedef struct DiskQuotaLocks DiskQuotaLocks;

/*
 * Key for one quota target in the shared quota state map.
 *
 * A target is unique only within a database and quota type, because namespace
 * OIDs and role OIDs can overlap.
 */
typedef struct DiskQuotaStateKey
{
	Oid			targetoid; /* namespace OID or role OID */
	Oid			databaseoid; /* database that owns this quota state */
	/* Distinguishes namespace quota entries from role quota entries. */
	uint32		targettype;
} DiskQuotaStateKey;

/*
 * Shared quota state used by backends to make enforcement decisions before the
 * worker has refreshed the older blacklist view.
 */
typedef struct DiskQuotaStateEntry
{
	DiskQuotaStateKey key; /* quota target identity */
	int64		limit_bytes; /* configured hard quota in bytes */
	int64		used_bytes; /* latest known usage for this target */
	/* Bytes reserved by active writers but not necessarily visible to worker. */
	int64		reserved_bytes;
} DiskQuotaStateEntry;

typedef struct DiskQuotaDbOidPids
{
	Oid dboid;
	int pid;
}DiskQuotaDbOidPids;

typedef struct DiskQuotaSessionPids
{
	int pid;
	Oid dbid;
	Latch		*procLatch;
	bool deal;
	struct DiskQuotaSessionPids *use_next;
	struct DiskQuotaSessionPids *use_prev;
	struct DiskQuotaSessionPids *free_next;
	struct DiskQuotaSessionPids *free_prev;
}DiskQuotaSessionPids;

/*
 * MessageBox is used to store a message for communication between
 * the diskquota launcher process and backends.
 * When backend create an extension, it send a message to launcher
 * to start the diskquota worker process and write the corresponding
 * dbOid into diskquota diskquota_database_list table in postgres database.
 * When backend drop an extension, it will send a message to launcher
 * to stop the diskquota worker process and remove the dbOid from diskquota
 * diskquota_database_list table as well.
 */
typedef struct DiskquotaMessageBox
{
	int launcher_pid;
	int req_pid;		/* pid of the request process */
	int cmd;			/* message command type, see MessageCommand */
	int result;			/* message result writen by launcher, see MessageResult */
	int data[4];		/* for create/drop extension diskquota, data[0] is dbid */
	int db_num;
	DiskQuotaDbOidPids dboid_pid_info[QUOTA_MAX_DB_NUM];
	DiskQuotaSessionPids *free_pid_list;
	DiskQuotaSessionPids *session_pid_list;
	DiskQuotaSessionPids session_pid_arry[0];
}DiskquotaMessageBox;

typedef enum MessageCommand
{
	CMD_INVILD = 0,
	CMD_CREATE_EXTENSION = 1,
	CMD_DROP_EXTENSION,
}DiskquotaMessageCommand;

typedef enum MessageResult
{
	ERR_PENDING = 0,
	ERR_OK,
	/* the number of database exceeds the maximum */
	ERR_EXCEED,
	/* add the dbid to hg_security.diskquota_database_list failed */
	ERR_ADD_TO_DB,
	/* cann't start worker process */
	ERR_START_WORKER,
	/* invalid dbid */
	ERR_INVALID_DBID,
	ERR_UNKNOWN,
}DiskquotaMessageResult;

extern DiskQuotaLocks diskquota_locks;
extern LWLockPadded *disk_quota_lock_array;

extern DiskquotaMessageBox *message_box;
extern HTAB *disk_quota_state_map;

extern quotaSwitch diskquota_enable;
extern quotaSwitch diskquota_strict;

extern DiskQuotaSessionPids *tail;

extern bool diskquotasql;

/* enforcement interface*/
extern void init_disk_quota_enforcement(void);
extern void diskquota_invalidate_db(Oid dbid);

/* quota model interface*/
extern void init_disk_quota_shmem(void);
extern void init_disk_quota_model(void);
extern void refresh_disk_quota_model(bool force);
//extern bool quota_check_common(Oid reloid);
extern bool quota_check_common_backlist_by_reloid(Oid reloid);
extern bool quota_check_common_backlist(Relation reln);

/* quotaspi interface */
extern void init_disk_quota_hook(void);
extern void delete_pid_from_queue(DiskQuotaSessionPids *node);

extern int64 get_current_usage_bytes(Oid targetoid, QuotaType type);
extern void reconcile_min_quota_reserve_files(void);
extern void clear_min_quota_reserve_files(void);
extern void resize_min_quota_reserve_file(QuotaType type, Oid targetoid, int64 target_bytes);
extern int64 get_min_quota_reserve_file_size(QuotaType type, Oid targetoid); /* BUG#Z8153 */

extern void update_shared_quota_state(Oid targetoid, int64 quota_limit_mb, QuotaType type);
extern void refresh_relation_quota_state(Oid relid);
extern void remove_shared_quota_state(Oid targetoid, QuotaType type);

extern int   diskquota_naptime;
extern int   diskquota_max_active_tables;
extern int   diskquota_min_quota_free_space;

void
refresh_disk_quota_usage(bool force);


#endif
