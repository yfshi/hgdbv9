/*-------------------------------------------------------------------------
 *
 * hg_ddos_guard.c
 *	  Shared login and frontend SQL parser rate limiting.
 *
 * The extension identifies clients by the peer address and the username
 * supplied in the startup packet.  Login and SQL traffic use independent
 * shared hash tables, fixed windows, and lock states.
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "fmgr.h"
#include "funcapi.h"
#include "access/htup_details.h"
#include "access/xact.h"
#include "common/hashfn.h"
#include "lib/stringinfo.h"
#include "libpq/auth.h"
#include "libpq/libpq-be.h"
#include "miscadmin.h"
#include "pgstat.h"
#include "postmaster/bgworker.h"
#include "postmaster/interrupt.h"
#include "storage/fd.h"
#include "storage/ipc.h"
#include "storage/latch.h"
#include "storage/lwlock.h"
#include "storage/shmem.h"
#include "tcop/tcopprot.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/hsearch.h"
#include "utils/inet.h"
#include "utils/acl.h"
#include "utils/rel.h"
#include "utils/timestamp.h"
#include "se_features_interface.h"

PG_MODULE_MAGIC;

/* The partition count must remain a power
 * of two for dynahash partitioning.
 */
#define DDOS_PARTITIONS 32

/* Interval in milliseconds between background cleanup passes. */
#define DDOS_CLEANUP_INTERVAL_MS (120 * 1000L)
#define DDOS_STATE_MAGIC ((uint32) 0x44444f53U)
#define DDOS_STATE_VERSION 1
#define DDOS_STATE_FILE "pg_stat/hg_ddos_lock.state"
#define DDOS_STATE_TMP_FILE "pg_stat/hg_ddos_lock.state.tmp"
#define DDOS_STATE_LEGACY_FILE "pg_stat/ddos_lock.state"

/* Address representation stored in a DdosKey. */
typedef enum DdosAddrType
{
	DDOS_ADDR_LOCAL = 0,
	DDOS_ADDR_IPV4,
	DDOS_ADDR_IPV6
} DdosAddrType;

/*
 * Hash key for one protected client identity.
 *
 * The address array stores four bytes for IPv4 and sixteen
 * bytes for IPv6. Zero-filling the complete key makes byte-wise
 * hashing deterministic.
 */
typedef struct DdosKey
{
	/* Database owning an SQL key, or InvalidOid for a login key. */
	Oid			database_id;

	/* Role OID, or InvalidOid when the startup user does not exist. */
	Oid			user_id;

	/* DdosAddrType value describing addr. */
	uint8		addr_type;

	/* IPv4 or IPv6 address in network byte order. */
	unsigned char addr[16];

	/* Startup-packet username. */
	char		username[NAMEDATALEN];
} DdosKey;

/*
 * One fixed-window entry in either the login or SQL shared hash table.
 */
typedef struct DdosEntry
{
	/* Hash key identifying the client. */
	DdosKey		key;

	/* True while requests must be rejected. */
	bool		locked;

	/* Time at which the lock was created. */
	TimestampTz lock_start;

	/* Time at which the lock expires. */
	TimestampTz lock_until;

	/* Time of the most recently counted request. */
	TimestampTz last_seen;

	/* Time at which the current counting window started. */
	TimestampTz window_start;

	/* Number of requests counted in the current window. */
	uint32		request_count;
} DdosEntry;

/* Persistent record types stored in ddos_lock.state. */
typedef enum DdosStateType
{
	DDOS_STATE_LOGIN = 1,
	DDOS_STATE_SQL
} DdosStateType;

/* On-disk header protecting the state record array. */
typedef struct DdosStateHeader
{
	uint32 magic;			/* DDOS_STATE_MAGIC. */
	uint32 version;		/* DDOS_STATE_VERSION. */
	uint32 record_count;	/* Number of records following this header. */
	uint32 checksum;	/* CRC-32C of all state records. */
} DdosStateHeader;

/* One currently locked shared-hash entry written to disk. */
typedef struct DdosStateRecord
{
	uint8 type;			/* DdosStateType value. */
	DdosKey key;		/* Locked client identity. */
	TimestampTz lock_start; /* Time at which the lock started. */
	TimestampTz lock_until; /* Time at which the lock expires. */
	uint32 request_count; /* Count present when the lock was created. */
} DdosStateRecord;

/* Counters and flags shared by every server process. */
typedef struct DdosShared
{
	/* True after shared objects are attached. */
	bool		initialized;

	/* Rejected login request count. */
	pg_atomic_uint64 login_reject_total;

	/* Rejected SQL request count. */
	pg_atomic_uint64 sql_reject_total;

} DdosShared;

/* Shared-memory objects initialized by ddos_shmem_startup(). */
static DdosShared *shared;		/* Extension-wide shared control block. */
static HTAB *login_hash;		/* Shared login-window hash table. */
static HTAB *sql_hash;			/* Shared SQL-window hash table. */
static LWLockPadded *login_locks; /* Locks protecting login hash partitions. */
static LWLockPadded *sql_locks;	/* Locks protecting SQL hash partitions. */
static LWLock *state_lock;		/* Serializes state snapshots and writes. */

/* Accepted values for the three DDoS feature switches. */
static const struct config_enum_entry ddos_switch_options[] =
{
	{"off", 0, false},
	{"on", 1, false},
	{NULL, 0, false}
};

/* GUC-backed configuration values; durations are expressed in milliseconds. */
static int ddos_enable = 0;	/* Master switch for all guard logic. */
static int login_enable = 0; /* Switch for login request accounting. */
static int sql_enable = 0;	/* Switch for SQL request accounting. */
static int login_window = 60000; /* Login fixed-window width. */
static int login_threshold = 100; /* Maximum login requests per window. */
static int login_lock_duration = 86400000; /* Login lock duration. */
static int login_ip_threshold = 0; /* Locked users required to lock an IP. */
static int login_ip_lock_duration = 86400000; /* Login IP lock duration. */
static int sql_window = 60000;	/* SQL fixed-window width. */
static int sql_threshold = 100;	/* Maximum SQL requests per window. */
static int sql_lock_duration = 86400000; /* SQL lock duration. */
static int max_login_entries = 10000; /* Login hash capacity. */
static int max_sql_entries = 10000; /* SQL hash capacity. */

/* Previously installed hooks are preserved to support hook chaining. */
static shmem_startup_hook_type prev_shmem_startup_hook; /* Prior shmem hook. */
static ClientPreAuthentication_hook_type prev_pre_auth_hook; /* Prior auth hook. */
static ClientQueryPreParse_hook_type prev_pre_parse_hook; /* Prior pre-parse hook. */
static ClientQueryPostParse_hook_type prev_post_parse_hook; /* Prior post-parse hook. */

/* Per-backend state carried from the pre-parse hook to the post-parse hook. */
static bool sql_overflow_pending; /* True when this parse crossed the limit. */
static DdosKey sql_pending_key;	/* Key associated with the pending rejection. */
static int64 sql_pending_count; /* Count that crossed the SQL threshold. */

void _PG_init(void);
void _PG_fini(void);
PGDLLEXPORT void hg_ddos_guard_cleanup_main(Datum main_arg) pg_attribute_noreturn();
PG_FUNCTION_INFO_V1(hg_ddos_unlock_login);
PG_FUNCTION_INFO_V1(hg_ddos_unlock_login_ip);
PG_FUNCTION_INFO_V1(hg_ddos_unlock_sql);
PG_FUNCTION_INFO_V1(hg_ddos_login_status);
PG_FUNCTION_INFO_V1(hg_ddos_login_ip_status);
PG_FUNCTION_INFO_V1(hg_ddos_sql_status);
PG_FUNCTION_INFO_V1(hg_ddos_stats);

static void state_save(void);
static uint32 state_checksum(const void *data, Size size);
static uint32 key_hash(const DdosKey *key);
static void hg_ddos_permission(void);

/*
 * state_checksum
 *
 * Compute CRC-32C locally so the extension does not depend on
 * PostgreSQL CRC symbols that are not exported on Windows.
 */
static uint32
state_checksum(const void *data, Size size)
{
	const unsigned char *position = (const unsigned char *) data;
	uint32 checksum = (uint32) 0xFFFFFFFFU;
	int bit;

	while (size-- > 0)
	{
		checksum ^= *position++;
		for (bit = 0; bit < 8; bit++)
		{
			if (checksum & 1)
			{
				checksum = (checksum >> 1) ^ (uint32) 0x82F63B78U;
			}
			else
			{
				checksum >>= 1;
			}
		}
	}

	return checksum ^ (uint32) 0xFFFFFFFFU;
}

/*
 * check_max_sql_entries
 *
 * Ensure that the SQL hash can contain more entries
 * than the maximum number of client connections.
 */
static bool
check_max_sql_entries(int *newval, void **extra, GucSource source)
{
	if (*newval <= MaxConnections)
	{
		GUC_check_errmsg("ddos.max_sql_entries must be greater than max_connections");
		return false;
	}

	return true;
}

/*
 * key_hash
 *
 * Compute the same hash value used to
 * select the entry partition.
 */
static uint32
key_hash(const DdosKey *key)
{
	return DatumGetUInt32(hash_any((const unsigned char *) key, sizeof(*key)));
}

/*
 * key_address_equal
 *
 * Return true when two keys contain
 * the same client address.
 */
static bool
key_address_equal(const DdosKey *left, const DdosKey *right)
{
	return left->addr_type == right->addr_type &&
		memcmp(left->addr, right->addr, sizeof(left->addr)) == 0;
}

/*
 * build_key
 *
 * Build a binary key from a backend connection descriptor.
 */
static void
build_key(Port *port, DdosKey *key)
{
	/* Initilaze key. */
	memset(key, 0, sizeof(*key));

	/* Set the user name to key. */
	strlcpy(key->username, port->user_name ? port->user_name : "",
			sizeof(key->username));
	key->user_id = get_role_oid(key->username, true);

	/* Set ip addr to key according to IP protocol. */
	if (port->raddr.addr.ss_family == AF_INET)
	{
		struct sockaddr_in *sin = (struct sockaddr_in *) &port->raddr.addr;
		key->addr_type = DDOS_ADDR_IPV4;
		memcpy(key->addr, &sin->sin_addr, 4);
	}
#ifdef HAVE_IPV6
	else if (port->raddr.addr.ss_family == AF_INET6)
	{
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *) &port->raddr.addr;
		key->addr_type = DDOS_ADDR_IPV6;
		memcpy(key->addr, &sin6->sin6_addr, 16);
	}
#endif
	else
	{
		key->addr_type = DDOS_ADDR_LOCAL;
	}
}

/*
 * key_address
 *
 * Convert a binary key address to a printable,
 * platform-independent form.
 */
static char *
key_address(const DdosKey *key, char *buf, Size size)
{
	if (key->addr_type == DDOS_ADDR_LOCAL)
		strlcpy(buf, "local", size);
	else if (key->addr_type == DDOS_ADDR_IPV4)
	{
		if (pg_inet_net_ntop(PGSQL_AF_INET, key->addr, 32, buf, size) == NULL)
		{
			ereport(ERROR,
					(errcode(ERRCODE_INTERNAL_ERROR),
					 errmsg("could not format IPv4 client address: %m")));
		}
	}
	else
	{
		if (pg_inet_net_ntop(PGSQL_AF_INET6, key->addr, 128, buf, size) == NULL)
		{
			ereport(ERROR,
					(errcode(ERRCODE_INTERNAL_ERROR),
					 errmsg("could not format IPv6 client address: %m")));
		}
	}
	return buf;
}

/*
 * parse_key
 *
 * Parse SQL function arguments into the canonical
 * binary key format.
 */
static bool
parse_key(text *address, const char *username, DdosKey *key)
{
	char *addr = text_to_cstring(address);

	memset(key, 0, sizeof(*key));

	/* Construct the key value. */
	strlcpy(key->username, username, sizeof(key->username));
	key->user_id = get_role_oid(key->username, true);
	if (strcmp(addr, "local") == 0)
	{
		key->addr_type = DDOS_ADDR_LOCAL;
	}
	else if (pg_inet_net_pton(PGSQL_AF_INET, addr, key->addr, -1) == 32)
	{
		key->addr_type = DDOS_ADDR_IPV4;
	}
	else if (pg_inet_net_pton(PGSQL_AF_INET6, addr, key->addr, -1) == 128)
	{
		key->addr_type = DDOS_ADDR_IPV6;
	}
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("invalid client IP address: %s", addr)));
	}

	return true;
}

/*
 * state_write_all
 *
 * Transfer an exact byte count, retrying interrupted system calls.
 */
static void
state_write_all(int fd, const void *buffer, Size size)
{
	const char *position = (const char *) buffer;
	Size remaining = size;

	while (remaining > 0)
	{
		ssize_t written = write(fd, position, remaining);

		/* Continue if the writ operation is interrupted. */
		if (written < 0 && errno == EINTR)
		{
			continue;
		}

		/* It wasn't interrupted; if the length of the data
		 * writen is less than 0, an error has occurred.
		 */
		if (written <= 0)
		{
			ereport(ERROR,
					(errcode_for_file_access(),
					errmsg("could not write DDoS state file: %m")));
		}

		position += written;
		remaining -= written;
	}
}

/*
 * state_read_all
 *
 * Read an exact byte count and reject truncated state files.
 */
static bool
state_read_all(int fd, void *buffer, Size size)
{
	char *position = (char *) buffer;
	Size remaining = size;

	while (remaining > 0)
	{
		ssize_t bytes_read = read(fd, position, remaining);

		/* Continue if the read operation is interrupted. */
		if (bytes_read < 0 && errno == EINTR)
		{
			continue;
		}

		/* It wasn't interrupted; if the length of the data
		 * read is less than 0, an error has occurred.
		 */
		if (bytes_read <= 0)
		{
			return false;
		}

		position += bytes_read;
		remaining -= bytes_read;
	}

	return true;
}

/*
 * state_save
 *
 * Rewrite the complete set of active locks
 * using an atomic rename.
 */
static void
state_save(void)
{
	DdosStateRecord *records;
	DdosStateHeader header;
	HASH_SEQ_STATUS scan;
	DdosEntry *entry;
	TimestampTz now = GetCurrentTimestamp();
	uint32 count = 0;
	int partition;
	int fd;
	char path[MAXPGPATH];
	char temporary[MAXPGPATH];

	/* Allocate a sufficiently large block of memory. */
	records = palloc(sizeof(DdosStateRecord) *
					 (max_login_entries + max_sql_entries));

	/* Initialize variable. */
	memset(records, 0, sizeof(DdosStateRecord) *
		   (max_login_entries + max_sql_entries));

	/* locked. */
	LWLockAcquire(state_lock, LW_EXCLUSIVE);

	/* Lock the hash login_locks. */
	for (partition = 0; partition < DDOS_PARTITIONS; partition++)
	{
		LWLockAcquire(&login_locks[partition].lock, LW_SHARED);
	}

	/* Find locked records in the login hash table. */
	hash_seq_init(&scan, login_hash);
	while ((entry = hash_seq_search(&scan)) != NULL)
	{
		if (entry->locked && entry->lock_until > now)
		{
			records[count].type = DDOS_STATE_LOGIN;
			records[count].key = entry->key;
			records[count].lock_start = entry->lock_start;
			records[count].lock_until = entry->lock_until;
			records[count].request_count = entry->request_count;
			count++;
		}
	}

	/* release the hash login_locks. */
	for (partition = DDOS_PARTITIONS - 1; partition >= 0; partition--)
	{
		LWLockRelease(&login_locks[partition].lock);
	}

	/* Lock the hash sql_locks. */
	for (partition = 0; partition < DDOS_PARTITIONS; partition++)
	{
		LWLockAcquire(&sql_locks[partition].lock, LW_SHARED);
	}

	/* Find locked records in the login hash table. */
	hash_seq_init(&scan, sql_hash);
	while ((entry = hash_seq_search(&scan)) != NULL)
	{
		if (entry->locked && entry->lock_until > now)
		{
			records[count].type = DDOS_STATE_SQL;
			records[count].key = entry->key;
			records[count].lock_start = entry->lock_start;
			records[count].lock_until = entry->lock_until;
			records[count].request_count = entry->request_count;
			count++;
		}
	}

	/* release the hash sql_locks. */
	for (partition = DDOS_PARTITIONS - 1; partition >= 0; partition--)
	{
		LWLockRelease(&sql_locks[partition].lock);
	}

	/* Initialize header. */
	memset(&header, 0, sizeof(header));

	/*
	 * Prepare header data for verification
	 * during recovery.
	 */
	header.magic = DDOS_STATE_MAGIC;
	header.version = DDOS_STATE_VERSION;
	header.record_count = count;
	header.checksum = state_checksum(records,
								 count * sizeof(DdosStateRecord));

	/* Prepare file path. */
	snprintf(path, sizeof(path), "%s/%s", DataDir, DDOS_STATE_FILE);
	snprintf(temporary, sizeof(temporary), "%s/%s", DataDir,
			 DDOS_STATE_TMP_FILE);

	/* Open the temporary file. */
	fd = OpenTransientFile(temporary,
					   O_WRONLY | O_CREAT | O_TRUNC | PG_BINARY);
	if (fd < 0)
	{
		ereport(ERROR,
				(errcode_for_file_access(),
				errmsg("could not create DDoS state file \"%s\": %m", temporary)));
	}

	/*
	 * Verification information is now written to
	 * the header of the temporary file.
	 */
	state_write_all(fd, &header, sizeof(header));

	/* Write the locked records to a file. */
	if (count > 0)
	{
		state_write_all(fd, records, count * sizeof(DdosStateRecord));
	}

	/* Persist static data to a file. */
	if (pg_fsync(fd) != 0 || CloseTransientFile(fd) != 0)
	{
		LWLockRelease(state_lock);
		pfree(records);

		ereport(ERROR,
				(errcode_for_file_access(),
				errmsg("could not sync DDoS state file: %m")));
	}

	/* Rename the temporary file to the official filename. */
	(void) durable_rename(temporary, path, ERROR);

	/* Release lock. */
	LWLockRelease(state_lock);

	pfree(records);
}

/*
 * state_load
 *
 * Restore every unexpired lock before the
 * postmaster accepts connections.
 */
static void
state_load(void)
{
	DdosStateHeader header;
	DdosStateRecord *records; /* Current-format state record array. */
	uint32 checksum;
	TimestampTz now = GetCurrentTimestamp();
	char path[MAXPGPATH];
	int fd;
	uint32 index;

	/* Prepare file path. */
	snprintf(path, sizeof(path), "%s/%s", DataDir, DDOS_STATE_FILE);

	/* Open the file, and return that
	 * the file is not exists.
	 */
	fd = OpenTransientFile(path, O_RDONLY | PG_BINARY);
	if (fd < 0 && errno == ENOENT)
	{
		snprintf(path, sizeof(path), "%s/%s", DataDir,
				 DDOS_STATE_LEGACY_FILE);
		fd = OpenTransientFile(path, O_RDONLY | PG_BINARY);
		if (fd < 0 && errno == ENOENT)
		{
			return;
		}
	}

	/* When read file header failure, need to report a error. */
	if (fd < 0 || !state_read_all(fd, &header, sizeof(header)))
	{
		ereport(ERROR,
				(errcode_for_file_access(),
				errmsg("could not read DDoS state file \"%s\": %m", path)));
	}

	/* If the checksum header is incorrect,
	 * it immediately throws a fit.
	 */
	if (header.magic != DDOS_STATE_MAGIC ||
		header.version != DDOS_STATE_VERSION ||
		header.record_count > (uint32) (max_login_entries + max_sql_entries))
	{
		ereport(ERROR,
				(errcode(ERRCODE_DATA_CORRUPTED),
				 errmsg("invalid DDoS state file header")));
	}

	/* Space prepared for reading out file data. */
	records = palloc(header.record_count * sizeof(DdosStateRecord));

	/* Read the file data to Memory. report 
	 * an error if read faild.
	 */
	if (header.record_count > 0 &&
		!state_read_all(fd, records,
						header.record_count * sizeof(DdosStateRecord)))
	{
		ereport(ERROR,
				(errcode(ERRCODE_DATA_CORRUPTED),
				 errmsg("truncated DDoS state file")));
	}

	/* close the ddos save file. */
	if (CloseTransientFile(fd) != 0)
	{
		ereport(ERROR,
				(errcode_for_file_access(),
				errmsg("could not close DDoS state file: %m")));
	}

	/* checksum those data. */
	checksum = state_checksum(records,
						  header.record_count * sizeof(DdosStateRecord));
	if (checksum != header.checksum)
	{
		ereport(ERROR,
				(errcode(ERRCODE_DATA_CORRUPTED),
				 errmsg("DDoS state file checksum mismatch")));
	}

	/* Sequentially insert the records read
	 * from the file into the hash table.
	 */
	for (index = 0; index < header.record_count; index++)
	{
		DdosStateRecord *record = &records[index]; /* Record being restored. */
		HTAB *hash;
		DdosEntry *restored;
		bool found;
		uint32 hashvalue;

		if (record->type != DDOS_STATE_LOGIN && record->type != DDOS_STATE_SQL)
		{
			ereport(ERROR,
					(errcode(ERRCODE_DATA_CORRUPTED),
					 errmsg("invalid record type in DDoS state file")));
		}

		if (record->lock_until <= now)
		{
			continue;
		}

		/* According to the record, choice on
		 * hash from login_hash and sql_hash.
		 */
		hash = record->type == DDOS_STATE_LOGIN ? login_hash : sql_hash;
		hashvalue = key_hash(&record->key);

		/* Directly insert a record; return NULL
		 * if the insertion fails.
		 */
		restored = hash_search_with_hash_value(hash, &record->key, hashvalue,
											HASH_ENTER_NULL, &found);
		if (restored == NULL)
		{
			ereport(ERROR,
					(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
					 errmsg("DDoS shared hash is full during recovery")));
		}

		/* If no record exists but a new one has been
		 * inserted, assign a value to that new record.
		 */
		if (!found)
		{
			memset(((char *) restored) + sizeof(DdosKey), 0,
				   sizeof(DdosEntry) - sizeof(DdosKey));
		}
		restored->locked = true;
		restored->lock_start = record->lock_start;
		restored->lock_until = record->lock_until;
		restored->request_count = record->request_count;
	}

	/* Free the tempory data in the Memory. */
	pfree(records);
}

/*
 * reserve_request
 *
 * Return -1 when the identity is already locked or when the
 * bounded hash is full. Otherwise return the request count
 * after inserting the new event. Set new_lock when this request
 * reaches the configured threshold.
 */
static int64
reserve_request(HTAB *hash, LWLockPadded *locks, const DdosKey *key,
				int window_ms, int threshold, int lock_ms,
				bool *new_lock, bool *auto_unlocked)
{
	uint32 hv = key_hash(key);	/* Calculate hash value. */
	LWLock *lock = &locks[hv % DDOS_PARTITIONS].lock;	/* Determine partition locks. */
	DdosEntry *entry;
	bool found;
	TimestampTz now = GetCurrentTimestamp();
	int64 result;

	/* All inspection and mutation of an
	 * entry uses its partition lock.
	 */
	*new_lock = false;
	*auto_unlocked = false;

	/* An exclusive lock is required here
	 * because the record might be modified.
	 */
	LWLockAcquire(lock, LW_EXCLUSIVE);

	/* Look up the record; create one if
	 * it doesn't exist, or return NULL
	 * if full.
	 */
	entry = hash_search_with_hash_value(hash, key, hv, HASH_ENTER_NULL, &found);
	if (entry == NULL)
	{
		LWLockRelease(lock);

		/* fail closed when the bounded hash is full */
		return -1;
	}

	if (!found)
	{
		/* dynahash has already copied the key;
		 * initialize the value fields.
		 */
		memset(((char *) entry) + sizeof(DdosKey), 0,
			   sizeof(DdosEntry) - sizeof(DdosKey));
	}

	/* No action is taken during the lock period. */
	if (entry->locked && now < entry->lock_until)
	{
		LWLockRelease(lock);
		return -1;
	}

	/* Unlocks upon expiration after being locked. */
	if (entry->locked)
	{
		/* Expiration starts a completely new window. */
		*auto_unlocked = true;
		entry->locked = false;
		entry->window_start = 0;
		entry->request_count = 0;
	}

	/* Start a new fixed counting window when the old one expires. */
	if (entry->window_start == 0 ||
		now >= entry->window_start + ((int64) window_ms * 1000))
	{
		entry->window_start = now;
		entry->request_count = 0;
	}

	entry->request_count++;
	result = entry->request_count;

	/* Time of the most recently counted request. */
	entry->last_seen = now;

	/* If a new lock is triggered, lock it immediately. */
	if (result >= threshold)
	{
		entry->locked = true;
		entry->lock_start = now;
		entry->lock_until = now + ((int64) lock_ms * 1000);
		*new_lock = true;
	}

	LWLockRelease(lock);

	return result;
}

/*
 * login_ip_is_locked
 *
 * Check a pure IP login lock without counting
 * another request.
 */
static bool
login_ip_is_locked(const DdosKey *key, bool *auto_unlocked)
{
	uint32 hv = key_hash(key); /* Calculate hash value. */
	LWLock *lock = &login_locks[hv % DDOS_PARTITIONS].lock; /* Determine partition lock. */
	DdosEntry *entry;
	TimestampTz now = GetCurrentTimestamp();
	bool locked = false;

	*auto_unlocked = false;
	LWLockAcquire(lock, LW_EXCLUSIVE);
	entry = hash_search_with_hash_value(login_hash, key, hv, HASH_FIND, NULL);
	if (entry != NULL)
	{
		if (entry->locked && now < entry->lock_until)
		{
			locked = true;
		}
		else
		{
			(void) hash_search_with_hash_value(login_hash, key, hv,
										 HASH_REMOVE, NULL);
			*auto_unlocked = true;
		}
	}
	LWLockRelease(lock);

	return locked;
}

/*
 * lock_login_ip_if_needed
 *
 * Lock a pure IP key when enough different users
 * are currently locked.
 */
static bool
lock_login_ip_if_needed(const DdosKey *ip_key)
{
	HASH_SEQ_STATUS scan; /* Shared hash scan state. */
	DdosEntry *entry; /* Entry currently considered. */
	DdosEntry *ip_entry;
	TimestampTz now = GetCurrentTimestamp();
	uint32 locked_users = 0;
	uint32 hashvalue = key_hash(ip_key);
	bool found;
	bool new_lock = false;
	int partition;

	if (login_ip_threshold == 0)
	{
		return false;
	}

	for (partition = 0; partition < DDOS_PARTITIONS; partition++)
	{
		LWLockAcquire(&login_locks[partition].lock, LW_EXCLUSIVE);
	}

	hash_seq_init(&scan, login_hash);
	while ((entry = hash_seq_search(&scan)) != NULL)
	{
		if (entry->key.username[0] != '\0' &&
			entry->locked && entry->lock_until > now &&
			key_address_equal(&entry->key, ip_key))
		{
			locked_users++;
		}
	}

	if (locked_users >= (uint32) login_ip_threshold)
	{
		ip_entry = hash_search_with_hash_value(login_hash, ip_key, hashvalue,
										  HASH_ENTER_NULL, &found);
		if (ip_entry == NULL)
		{
			ereport(WARNING,
					(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
					 errmsg("could not create login IP lock because the shared hash is full")));
		}
		else if (!found || !ip_entry->locked || ip_entry->lock_until <= now)
		{
			if (!found)
			{
				memset(((char *) ip_entry) + sizeof(DdosKey), 0,
					   sizeof(DdosEntry) - sizeof(DdosKey));
			}
			ip_entry->locked = true;
			ip_entry->lock_start = now;
			ip_entry->lock_until = now + ((int64) login_ip_lock_duration * 1000);
			ip_entry->last_seen = now;
			ip_entry->request_count = locked_users;
			new_lock = true;
		}
	}

	for (partition = DDOS_PARTITIONS - 1; partition >= 0; partition--)
	{
		LWLockRelease(&login_locks[partition].lock);
	}

	return new_lock;
}

/*
 * ddos_pre_authentication
 *
 * Reject excessive connection attempts before
 * PostgreSQL authenticates them.
 */
static void
ddos_pre_authentication(Port *port)
{
	DdosKey key;
	DdosKey ip_key;
	bool new_lock;
	bool auto_unlocked;
	int64 count;

	/* Perform additional actions in
	 * the pre-execution hook.
	 */
	if (prev_pre_auth_hook)
	{
		prev_pre_auth_hook(port);
	}

	/* Prerequisites that must be met. */
	if (!ddos_enable || !login_enable || shared == NULL)
	{
		return;
	}

	/* Construct the key value and look up the record. */
	build_key(port, &key);

	/* Construct the pure IP key and look up the record. */
	ip_key = key;
	ip_key.user_id = InvalidOid;
	memset(ip_key.username, 0, sizeof(ip_key.username));
	auto_unlocked = false;
	if (login_ip_threshold > 0 &&
		login_ip_is_locked(&ip_key, &auto_unlocked))
	{
		pg_atomic_fetch_add_u64(&shared->login_reject_total, 1);
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PASSWORD),
				 errmsg("password authentication failed for user \"%s\"",
						key.username)));
	}

	/* Updated the hash and synchronized the file. */
	if (auto_unlocked)
	{
		state_save();
	}

	count = reserve_request(login_hash, login_locks, &key, login_window,
							login_threshold, login_lock_duration, &new_lock,
							&auto_unlocked);

	/* Update files after automatic unlocking. */
	if (auto_unlocked)
	{
		state_save();
	}

	/* If a locked record is found, report an
	 * error; however, if a new lock needs to
	 * be added, add it to the file first before
	 * reporting the error.
	 */
	if (!OidIsValid(key.user_id) || count < 0 || new_lock)
	{
		pg_atomic_fetch_add_u64(&shared->login_reject_total, 1);
		if (new_lock)
		{
			(void)lock_login_ip_if_needed(&ip_key);
			state_save();
		}
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PASSWORD),
				 errmsg("password authentication failed for user \"%s\"",
						key.username)));
	}
}

/*
 * ddos_pre_parse
 *
 * Requests issued while an explicit transaction block is
 * active are not counted.  BEGIN itself is counted because
 * the block does not yet exist when its pre-parse hook runs.
 */
static void
ddos_pre_parse(const char *query_string)
{
	DdosKey key;
	bool new_lock;
	bool auto_unlocked;
	int64 count;

	/* Perform additional actions in the pre-execution hook. */
	if (prev_pre_parse_hook)
	{
		prev_pre_parse_hook(query_string);
	}

	/* A flag was set and the lock was created,
	 * but no error has been thrown yet.
	 */
	sql_overflow_pending = false;

	/* Prerequisites that must be met. */
	if (!ddos_enable || !sql_enable ||
		shared == NULL || MyProcPort == NULL)
	{
		return;
	}

	/* Everything after the request that began
	 * an explicit transaction is free.
	 */
	if (IsTransactionBlock())
	{
		return;
	}

	/* Construct the key value and look up the record. */
	build_key(MyProcPort, &key);
	key.database_id = MyDatabaseId;
	count = reserve_request(sql_hash, sql_locks, &key, sql_window,
							sql_threshold, sql_lock_duration,
							&new_lock, &auto_unlocked);

	/* Update files after automatic unlocking. */
	if (auto_unlocked)
	{
		state_save();
	}

	/* If a locked record is found, report an
	 * error;
	 */
	if (count < 0)
	{
		pg_atomic_fetch_add_u64(&shared->sql_reject_total, 1);
		ereport(ERROR,
				(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
				errmsg("SQL request temporarily rejected")));
	}

	/* if a new lock needs to be added, add it to
	 * the file first before reporting the error.
	 */
	if (new_lock)
	{
		sql_overflow_pending = true;
		sql_pending_key = key;
		sql_pending_count = count;
		state_save();
	}
}

/*
 * finish_parse
 *
 * Reject the threshold-crossing SQL request after
 * raw parsing has completed.
 */
static void
finish_parse(void)
{
	/* An error occurs if a lock
	 * is applied beforehand.
	 */
	if (!sql_overflow_pending)
	{
		return;
	}

	/* Reset the variable. */
	sql_overflow_pending = false;
	pg_atomic_fetch_add_u64(&shared->sql_reject_total, 1);

	/* report an error. */
	ereport(ERROR,
			(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
			errmsg("SQL request temporarily rejected")));
}

/*
 * ddos_post_parse
 *
 * Chain the post-parse hook and then finalize any pending
 * rate-limit action.
 */
static void
ddos_post_parse(const char *q, List *trees)
{
	/* Perform additional actions in
	 * the pre-execution hook.
	 */
	if (prev_post_parse_hook)
	{
		prev_post_parse_hook(q, trees);
	}

	finish_parse();
}

/*
 * cleanup_hash
 *
 * Remove expired locks and entries whose
 * fixed window has expired.
 */
static bool
cleanup_hash(HTAB *hash, LWLockPadded *locks, int window_ms)
{
	HASH_SEQ_STATUS scan;			/* Sequential scan over the protected hash. */
	DdosEntry *entry;				/* Entry currently considered for removal. */
	TimestampTz now = GetCurrentTimestamp();	/* Cleanup reference time. */
	bool removed_lock = false;		/* True when persistent lock state changed. */
	int partition;					/* Partition-lock loop index. */

	/* Lock all partitions of the hash
	 * in preparation for cleaning up
	 * long-inactive hash entries.
	 */
	for (partition = 0; partition < DDOS_PARTITIONS; partition++)
	{
		LWLockAcquire(&locks[partition].lock, LW_EXCLUSIVE);
	}

	/* Preparing to start a sequential scan of the hash table. */
	hash_seq_init(&scan, hash);
	while ((entry = hash_seq_search(&scan)) != NULL)
	{
		bool remove_entry; /* True when this entry no longer carries state. */

		if (entry->locked)
		{
			/* If anything needs to be
			 * unlocked, simply mark it.
			 */
			remove_entry = entry->lock_until <= now;
			if (remove_entry)
			{
				removed_lock = true;
			}
		}
		else
		{
			/* First, identify the unused
			 * records, then delete them.
			 */
			remove_entry = entry->last_seen == 0 ||
				now > entry->last_seen + ((int64) window_ms * 1000);
		}

		/* Delete idle records. */
		if (remove_entry)
		{
			(void) hash_search_with_hash_value(hash, &entry->key,
										 key_hash(&entry->key), HASH_REMOVE, NULL);
		}
	}

	/* Release all partitions of the hash lock. */
	for (partition = DDOS_PARTITIONS - 1; partition >= 0; partition--)
	{
		LWLockRelease(&locks[partition].lock);
	}

	return removed_lock;
}

/*
 * hg_ddos_guard_cleanup_main
 *
 * Periodically clean shared hashes without connecting to a database.
 */
PGDLLEXPORT void
hg_ddos_guard_cleanup_main(Datum main_arg)
{
	pqsignal(SIGHUP, SignalHandlerForConfigReload);
	pqsignal(SIGTERM, die);
	BackgroundWorkerUnblockSignals();

	for (;;)
	{
		bool persistent_change;		/* True when an expired lock was removed. */
		int wait_result;			/* Result returned by WaitLatch(). */

		CHECK_FOR_INTERRUPTS();
		if (ConfigReloadPending)
		{
			ConfigReloadPending = false;
			ProcessConfigFile(PGC_SIGHUP);
		}

		/* Clean up records that have been
		 * inactive for a long period; return
		 * true if any records require unlocking.
		 */
		persistent_change = cleanup_hash(login_hash, login_locks, login_window);
		persistent_change |= cleanup_hash(sql_hash, sql_locks, sql_window);

		/* If there are items to unlock,
		 * update the file records.
		 */
		if (persistent_change)
		{
			state_save();
		}

		wait_result = WaitLatch(MyLatch,
							  WL_LATCH_SET | WL_TIMEOUT | WL_EXIT_ON_PM_DEATH,
							  DDOS_CLEANUP_INTERVAL_MS,
							  PG_WAIT_EXTENSION);

		ResetLatch(MyLatch);
		if (wait_result & WL_LATCH_SET)
		{
			CHECK_FOR_INTERRUPTS();
		}

		/* Reload GUC parameters for the current process. */
		if (ConfigReloadPending)
		{
			ConfigReloadPending = false;
			ProcessConfigFile(PGC_SIGHUP);
		}
	}
}

/*
 * ddos_shmem_startup
 *
 * Allocate or attach all shared-memory
 * objects during server startup.
 */
static void
ddos_shmem_startup(void)
{
	bool found;		/* True when the shared control block already exists. */
	HASHCTL ctl;	/* Shared hash configuration. */

	/* Perform additional actions in
	 * the pre-execution hook.
	 */
	if (prev_shmem_startup_hook)
	{
		prev_shmem_startup_hook();
	}

	/* palloc hg_ddos_guard shared. */
	LWLockAcquire(AddinShmemInitLock, LW_EXCLUSIVE);
	shared = ShmemInitStruct("hg_ddos_guard shared", sizeof(DdosShared), &found);
	if (!found)
	{
		/* Set the initialize value. */
		memset(shared, 0, sizeof(*shared));
		pg_atomic_init_u64(&shared->login_reject_total, 0);
		pg_atomic_init_u64(&shared->sql_reject_total, 0);
	}

	/* Set the initialize value. */
	memset(&ctl, 0, sizeof(ctl));
	ctl.keysize = sizeof(DdosKey);
	ctl.entrysize = sizeof(DdosEntry);
	ctl.num_partitions = DDOS_PARTITIONS;

	/* Create hg_ddos_guard login hash. */
	login_hash = ShmemInitHash("hg_ddos_guard login hash",
							   max_login_entries, max_login_entries, &ctl,
							   HASH_ELEM | HASH_BLOBS | HASH_PARTITION);

	/* Create hg_ddos_guard sql hash. */
	sql_hash = ShmemInitHash("hg_ddos_guard sql hash",
							 max_sql_entries, max_sql_entries, &ctl,
							 HASH_ELEM | HASH_BLOBS | HASH_PARTITION);

	/* Acquire login, SQL limits, and status locks. */
	login_locks = GetNamedLWLockTranche("hg_ddos_guard_login");
	sql_locks = GetNamedLWLockTranche("hg_ddos_guard_sql");
	state_lock = &GetNamedLWLockTranche("hg_ddos_guard_state")[0].lock;

	/* Load the data from the file into a hash table. */
	if (!found)
	{
		state_load();
	}

	/* Set the flag of initialized. */
	shared->initialized = true;

	LWLockRelease(AddinShmemInitLock);
}

/*
 * ddos_memsize
 *
 * Calculate the shared-memory reservation
 * for both bounded hashes.
 */
static Size
ddos_memsize(void)
{
	return MAXALIGN(sizeof(DdosShared)) +
		hash_estimate_size(max_login_entries, sizeof(DdosEntry)) +
		hash_estimate_size(max_sql_entries, sizeof(DdosEntry));
}

/*
 * _PG_init
 *
 * Define settings, reserve shared
 * resources, and install hooks.
 */
void
_PG_init(void)
{
	BackgroundWorker worker; /* Cleanup worker registration descriptor. */

	if (!process_shared_preload_libraries_in_progress)
	{
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("hg_ddos_guard must be loaded via shared_preload_libraries")));
	}

	DefineCustomEnumVariable("ddos.enable",
							 "Enable all DDoS guards.",
							 NULL,
							 &ddos_enable,
							 0,
							 ddos_switch_options,
							 PGC_SIGHUP,
							 0,
							 NULL,
							 NULL,
							 NULL);

	DefineCustomEnumVariable("ddos.login_enable",
							 "Enable login guard.",
							 NULL,
							 &login_enable,
							 0,
							 ddos_switch_options,
							 PGC_SIGHUP,
							 0,
							 NULL,
							 NULL,
							 NULL);

	DefineCustomEnumVariable("ddos.sql_enable",
							 "Enable SQL guard.",
							 NULL,
							 &sql_enable,
							 0,
							 ddos_switch_options,
							 PGC_SIGHUP,
							 0,
							 NULL,
							 NULL,
							 NULL);

	DefineCustomIntVariable("ddos.login_window",
						"Fixed login window.",
						NULL,
						&login_window,
						60000,
						1,
						86400000,
						PGC_SIGHUP,
						GUC_UNIT_MS,
						NULL,
						NULL,
						NULL);

	DefineCustomIntVariable("ddos.login_threshold",
						"Login request threshold.",
						NULL,
						&login_threshold,
						100,
						1,
						144000,
						PGC_SIGHUP,
						0,
						NULL,
						NULL,
						NULL);

	DefineCustomIntVariable("ddos.login_lock_duration",
						"Login lock duration.",
						NULL,
						&login_lock_duration,
						86400000,
						86400000,
						604800000,
						PGC_SIGHUP,
						GUC_UNIT_MS,
						NULL,
						NULL,
						NULL);

	DefineCustomIntVariable("ddos.login_ip_threshold",
						"Locked users required to lock a client IP.",
						NULL,
						&login_ip_threshold,
						0,
						0,
						1000,
						PGC_SIGHUP,
						0,
						NULL,
						NULL,
						NULL);

	DefineCustomIntVariable("ddos.login_ip_lock_duration",
						"Login client IP lock duration.",
						NULL,
						&login_ip_lock_duration,
						86400000,
						86400000,
						604800000,
						PGC_SIGHUP,
						GUC_UNIT_MS,
						NULL,
						NULL,
						NULL);

	DefineCustomIntVariable("ddos.sql_window",
						"Fixed SQL window.",
						NULL,
						&sql_window,
						60000,
						1,
						86400000,
						PGC_SIGHUP,
						GUC_UNIT_MS,
						NULL,
						NULL,
						NULL);

	DefineCustomIntVariable("ddos.sql_threshold",
						"SQL parser request threshold.",
						NULL,
						&sql_threshold,
						100,
						1,
						144000,
						PGC_SIGHUP,
						0,
						NULL,
						NULL,
						NULL);

	DefineCustomIntVariable("ddos.sql_lock_duration",
						"SQL lock duration.",
						NULL,
						&sql_lock_duration,
						86400000,
						86400000,
						604800000,
						PGC_SIGHUP,
						GUC_UNIT_MS,
						NULL,
						NULL,
						NULL);

	DefineCustomIntVariable("ddos.max_login_entries",
						"Maximum login keys.",
						NULL,
						&max_login_entries,
						10000,
						16,
						1000000,
						PGC_POSTMASTER,
						GUC_DISALLOW_IN_AUTO_FILE,
						check_max_sql_entries,
						NULL,
						NULL);

	DefineCustomIntVariable("ddos.max_sql_entries",
						"Maximum SQL keys.",
						NULL,
						&max_sql_entries,
						10000,
						16,
						1000000,
						PGC_POSTMASTER,
						GUC_DISALLOW_IN_AUTO_FILE,
						check_max_sql_entries,
						NULL,
						NULL);

	RequestAddinShmemSpace(ddos_memsize());
	RequestNamedLWLockTranche("hg_ddos_guard_login", DDOS_PARTITIONS);
	RequestNamedLWLockTranche("hg_ddos_guard_sql", DDOS_PARTITIONS);
	RequestNamedLWLockTranche("hg_ddos_guard_state", 1);
	prev_shmem_startup_hook = shmem_startup_hook;
	shmem_startup_hook = ddos_shmem_startup;
	prev_pre_auth_hook = ClientPreAuthentication_hook;
	ClientPreAuthentication_hook = ddos_pre_authentication;
	prev_pre_parse_hook = ClientQueryPreParse_hook;
	ClientQueryPreParse_hook = ddos_pre_parse;
	prev_post_parse_hook = ClientQueryPostParse_hook;
	ClientQueryPostParse_hook = ddos_post_parse;

	memset(&worker, 0, sizeof(worker));
	worker.bgw_flags = BGWORKER_SHMEM_ACCESS;
	worker.bgw_start_time = BgWorkerStart_RecoveryFinished;
	worker.bgw_restart_time = 1;
	snprintf(worker.bgw_library_name, BGW_MAXLEN, "hg_ddos_guard");
	snprintf(worker.bgw_function_name, BGW_MAXLEN,
			 "hg_ddos_guard_cleanup_main");
	snprintf(worker.bgw_name, BGW_MAXLEN, "hg_ddos_guard cleaner");
	snprintf(worker.bgw_type, BGW_MAXLEN, "hg_ddos_guard cleaner");
	RegisterBackgroundWorker(&worker);
}

/*
 * _PG_fini
 *
 * Restore the hook chain when the module is unloaded.
 */
void
_PG_fini(void)
{
	shmem_startup_hook = prev_shmem_startup_hook;
	ClientPreAuthentication_hook = prev_pre_auth_hook;
	ClientQueryPreParse_hook = prev_pre_parse_hook;
	ClientQueryPostParse_hook = prev_post_parse_hook;
}

/*
 * unlock_common
 *
 * Remove one selected guard entry after a superuser check.
 */
static Datum
unlock_common(PG_FUNCTION_ARGS, HTAB *hash, LWLockPadded *locks,
			  Oid database_id)
{
	DdosKey key;		/* Canonical identity supplied by the caller. */
	uint32 hashvalue;	/* Hash value selecting the partition. */
	LWLock *lock;		/* Lock protecting the selected partition. */
	bool removed = false;		/* True when a matching entry was removed. */
	bool nonexistent_user = false; /* True when the removed key has no role OID. */

	/* Permission check. */
	hg_ddos_permission();

	/* Construct the hash key. */
	parse_key(PG_GETARG_TEXT_PP(0), NameStr(*PG_GETARG_NAME(1)), &key);
	key.database_id = database_id;

	/* Confirm Lock. */
	hashvalue = key_hash(&key);
	lock = &locks[hashvalue % DDOS_PARTITIONS].lock;

	/* Acquire an exclusive lock and
	 * delete data from the record.
	 */
	LWLockAcquire(lock, LW_EXCLUSIVE);
	removed = hash_search_with_hash_value(hash, &key, hashvalue,
										  HASH_REMOVE, NULL) != NULL;
	LWLockRelease(lock);
	if (removed && !OidIsValid(key.user_id))
	{
		nonexistent_user = true;
	}
	if (!removed && OidIsValid(key.user_id))
	{
		key.user_id = InvalidOid;
		hashvalue = key_hash(&key);
		lock = &locks[hashvalue % DDOS_PARTITIONS].lock;
		LWLockAcquire(lock, LW_EXCLUSIVE);
		removed = hash_search_with_hash_value(hash, &key, hashvalue,
										  HASH_REMOVE, NULL) != NULL;
		LWLockRelease(lock);
		if (removed)
		{
			nonexistent_user = true;
		}
	}

	/* If the record is removed from
	 * the hash, then delete it from
	 * the file as well.
	 */
	if (removed)
	{
		state_save();
		if (nonexistent_user)
		{
			ereport(NOTICE,
					(errcode(ERRCODE_UNDEFINED_OBJECT),
					 errmsg("unlocked a DDoS guard record for nonexistent user \"%s\"",
							key.username)));
		}
	}

	PG_RETURN_BOOL(removed);
}

/*
 * hg_ddos_unlock_login
 *
 * Clear a login lockout.
 */
Datum
hg_ddos_unlock_login(PG_FUNCTION_ARGS)
{
	return unlock_common(fcinfo, login_hash, login_locks, InvalidOid);
}

/*
 * hg_ddos_unlock_login_ip
 *
 * Clear a client IP login lockout.
 */
Datum
hg_ddos_unlock_login_ip(PG_FUNCTION_ARGS)
{
	DdosKey key; /* Canonical pure IP key supplied by the caller. */
	DdosKey *user_keys = NULL; /* Keys of locked users selected by all mode. */
	uint32 hashvalue; /* Hash value selecting the partition. */
	LWLock *lock; /* Lock protecting the selected partition. */
	HASH_SEQ_STATUS scan; /* Shared hash scan state. */
	DdosEntry *entry; /* Entry currently considered by all mode. */
	StringInfoData users; /* Names reported after records are unlocked. */
	TimestampTz now = GetCurrentTimestamp(); /* Current lock-state timestamp. */
	char *mode; /* Requested unlock scope. */
	char address[128]; /* Printable client address. */
	int user_count = 0; /* Number of locked user records selected. */
	int user_capacity = 0; /* Allocated user key slots. */
	int partition; /* Partition-lock loop index. */
	int index; /* Selected user loop index. */
	bool removed = false; /* True when any record is removed. */

	/* Permission check. */
	hg_ddos_permission();

	/* For the second parameter value, "all" indicates
	 * the IP address and all associated locked login
	 * records, while "only" indicates unlocking the IP
	 * address only.
	 */
	mode = text_to_cstring(PG_GETARG_TEXT_PP(1));
	if (strcmp(mode, "only") != 0 && strcmp(mode, "all") != 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("invalid login IP unlock mode: \"%s\"", mode),
				 errhint("Available values: all, only.")));
	}

	/* Construct the hash key. */
	parse_key(PG_GETARG_TEXT_PP(0), "", &key);
	key.database_id = InvalidOid;
	hashvalue = key_hash(&key);

	/*"all" indicates the IP address and all associated
	 * locked login records, while "only" indicates unlocking
	 * the IP address only.
	 */
	if (strcmp(mode, "only") == 0)
	{
		/* Unlock IP. */
		lock = &login_locks[hashvalue % DDOS_PARTITIONS].lock;
		LWLockAcquire(lock, LW_EXCLUSIVE);
		removed = hash_search_with_hash_value(login_hash, &key, hashvalue,
										 HASH_REMOVE, NULL) != NULL;
		LWLockRelease(lock);
	}
	else
	{
		/* Initialize a variable to store
		 * alarm information.
		 */
		initStringInfo(&users);

		/* First, lock the hash. */
		for (partition = 0; partition < DDOS_PARTITIONS; partition++)
		{
			LWLockAcquire(&login_locks[partition].lock, LW_EXCLUSIVE);
		}

		/* Perform a sequential hash scan to
		 * locate the record to be unlocked.
		 */
		hash_seq_init(&scan, login_hash);
		while ((entry = hash_seq_search(&scan)) != NULL)
		{
			if (entry->key.username[0] == '\0' || !entry->locked ||
				entry->lock_until <= now || !key_address_equal(&entry->key, &key))
			{
				continue;
			}

			if (user_count == user_capacity)
			{
				user_capacity = user_capacity == 0 ? 8 : user_capacity * 2;
				if (user_keys == NULL)
				{
					user_keys = palloc(sizeof(DdosKey) * user_capacity);
				}
				else
				{
					user_keys = repalloc(user_keys, sizeof(DdosKey) * user_capacity);
				}
			}

			user_keys[user_count++] = entry->key;

			/* Assemble alarm information. */
			appendStringInfo(&users, "%s%s", users.len == 0 ? "" : ", ",
							 entry->key.username);
		}

		/* Prioritize IP unlocking. */
		removed = hash_search_with_hash_value(login_hash, &key, hashvalue,
										 HASH_REMOVE, NULL) != NULL;

		/* Unlock users under the IP address. */
		for (index = 0; index < user_count; index++)
		{
			uint32 user_hashvalue = key_hash(&user_keys[index]);

			if (hash_search_with_hash_value(login_hash, &user_keys[index],
										user_hashvalue, HASH_REMOVE, NULL) != NULL)
			{
				removed = true;
			}
		}

		/* Release the hash lock. */
		for (partition = DDOS_PARTITIONS - 1; partition >= 0; partition--)
		{
			LWLockRelease(&login_locks[partition].lock);
		}

		/* If a user is unlocked, an
		 * alert must be issued.
		 */
		if (user_count > 0)
		{
			char	*ipaddr = key_address(&key, address, sizeof(address));

			elog(LOG, "unlocked login records for client IP \"%s\": %s",
				 ipaddr, users.data);

			ereport(WARNING,
					(errcode(ERRCODE_WARNING),
					 errmsg("unlocked login records for client IP \"%s\": %s",
							ipaddr, users.data)));
		}

		/* Release variables. */
		if (user_keys != NULL)
		{
			pfree(user_keys);
		}

		pfree(users.data);
	}

	if (removed)
	{
		state_save();
	}

	pfree(mode);

	PG_RETURN_BOOL(removed);
}

/*
 * hg_ddos_unlock_sql
 *
 * Clear a SQL execution lock.
 */
Datum
hg_ddos_unlock_sql(PG_FUNCTION_ARGS)
{
	return unlock_common(fcinfo, sql_hash, sql_locks, MyDatabaseId);
}

/*
 * status_common
 *
 * Materialize a stable snapshot of one guard hash.
 */
static Datum
status_common(PG_FUNCTION_ARGS, HTAB *hash, LWLockPadded *locks,
			  bool ip_only, bool current_database_only)
{
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;	/* SRF state. */
	TupleDesc descriptor;		/* Descriptor of each result row. */
	Tuplestorestate *store;		/* Materialized result store. */
	HASH_SEQ_STATUS scan;		/* Shared hash scan state. */
	DdosEntry *entry;			/* Entry currently copied to the result. */
	MemoryContext oldcontext;	/* Caller memory context. */
	int partition;				/* Partition-lock loop index. */

	/* Permission Check. */
	hg_ddos_permission();

	/* Confirm that the current invocation
	 * environment supports Materialize mode.
	 */
	if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo) ||
		!(rsinfo->allowedModes & SFRM_Materialize))
	{
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("materialize mode required")));
	}

	/* Construct return columns. */
	oldcontext = MemoryContextSwitchTo(rsinfo->econtext->ecxt_per_query_memory);
	if (get_call_result_type(fcinfo, NULL, &descriptor) != TYPEFUNC_COMPOSITE)
	{
		ereport(ERROR,
				(errcode(ERRCODE_DATATYPE_MISMATCH),
				 errmsg("return type must be a row type")));
	}
	descriptor = BlessTupleDesc(descriptor);
	store = tuplestore_begin_heap(true, false, work_mem);
	MemoryContextSwitchTo(oldcontext);

	/* Lock all hash partitions. */
	for (partition = 0; partition < DDOS_PARTITIONS; partition++)
	{
		LWLockAcquire(&locks[partition].lock, LW_SHARED);
	}

	/* Sequentially scan the hash,
	 * write the values ​​into the columns
	 * to be returned, and construct the
	 * row data.
	 */
	hash_seq_init(&scan, hash);
	while ((entry = hash_seq_search(&scan)) != NULL)
	{
		Datum values[8];	/* Values forming one status row. */
		bool nulls[8];		/* NULL flags. */
		char address[128]; /* Printable client address. */

		if (ip_only != (entry->key.username[0] == '\0'))
		{
			continue;
		}
		if (current_database_only && entry->key.database_id != MyDatabaseId)
		{
			continue;
		}

		MemSet(nulls, false, sizeof(nulls));
		values[0] = CStringGetTextDatum(key_address(&entry->key, address, sizeof(address)));
		values[1] = CStringGetTextDatum(entry->key.username);
		values[2] = ObjectIdGetDatum(entry->key.user_id);

		/* database id is invalid oid, return null. */
		if (OidIsValid(entry->key.database_id))
		{
			values[3] = ObjectIdGetDatum(entry->key.database_id);
		}
		else
		{
			nulls[3] = true;
			values[3] = (Datum) 0;
		}

		if (TupleDescAttr(descriptor, 4)->atttypid == INT8OID)
		{
			values[4] = Int64GetDatum(entry->request_count);
		}
		else if (TupleDescAttr(descriptor, 4)->atttypid == INT4OID)
		{
			values[4] = Int32GetDatum(Min(entry->request_count,
										(uint32) INT_MAX));
		}
		else
		{
			ereport(ERROR,
					(errcode(ERRCODE_DATATYPE_MISMATCH),
					 errmsg("request_count must be declared as integer or bigint")));
		}
		values[5] = BoolGetDatum(entry->locked);

		/* If locked, return the lock
		 * time; otherwise, return null.
		 */
		if (entry->locked)
		{
			values[6] = TimestampTzGetDatum(entry->lock_start);
			values[7] = TimestampTzGetDatum(entry->lock_until);
		}
		else
		{
			nulls[6] = nulls[7] = true;
			values[6] = values[7] = (Datum) 0;
		}

		tuplestore_putvalues(store, descriptor, values, nulls);
	}

	/* Unlock all hash partitions. */
	for (partition = DDOS_PARTITIONS - 1; partition >= 0; partition--)
	{
		LWLockRelease(&locks[partition].lock);
	}

	tuplestore_donestoring(store);
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = store;
	rsinfo->setDesc = descriptor;

	PG_RETURN_NULL();
}

/*
 * hg_ddos_login_status
 *
 * View the records in the login hash.
 */
Datum
hg_ddos_login_status(PG_FUNCTION_ARGS)
{
	return status_common(fcinfo, login_hash, login_locks, false, false);
}

/*
 * hg_ddos_login_ip_status
 *
 * View the pure IP records in the login hash.
 */
Datum
hg_ddos_login_ip_status(PG_FUNCTION_ARGS)
{
	return status_common(fcinfo, login_hash, login_locks, true, false);
}

/*
 * hg_ddos_sql_status
 *
 * View the records in the sql hash.
 */
Datum
hg_ddos_sql_status(PG_FUNCTION_ARGS)
{
	return status_common(fcinfo, sql_hash, sql_locks, false, true);
}

/*
 * hg_ddos_stats
 *
 * Return aggregate guard statistics.
 */
Datum
hg_ddos_stats(PG_FUNCTION_ARGS)
{
	TupleDesc descriptor; /* Descriptor inferred from OUT parameters. */
	HeapTuple tuple; /* Single statistics tuple returned to SQL. */
	Datum values[4]; /* Values forming the statistics tuple. */
	bool nulls[4] = {false, false, false, false}; /* Statistics NULL flags. */

	/* Permission Check. */
	hg_ddos_permission();

	/* Determining the type of the returned result. */
	if (get_call_result_type(fcinfo, NULL, &descriptor) != TYPEFUNC_COMPOSITE)
	{
		ereport(ERROR,
				(errcode(ERRCODE_DATATYPE_MISMATCH),
				 errmsg("return type must be a row type")));
	}

	/* Construct column values. */
	values[0] = Int64GetDatum((int64) hash_get_num_entries(login_hash));
	values[1] = Int64GetDatum((int64) hash_get_num_entries(sql_hash));
	values[2] = Int64GetDatum((int64)
		pg_atomic_read_u64(&shared->login_reject_total));
	values[3] = Int64GetDatum((int64)
		pg_atomic_read_u64(&shared->sql_reject_total));

	/* Construct a tuple. */
	tuple = heap_form_tuple(descriptor, values, nulls);

	/* Return a tuple. */
	PG_RETURN_DATUM(HeapTupleGetDatum(tuple));
}

/*
 * hg_ddos_permission
 *
 * hg_ddos_guard function and view permission check.
 */
static void
hg_ddos_permission(void)
{
	if (se_feature_mode)
	{
		if (!IS_SYSSSO(GetUserId()))
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					errmsg("permission denied.")));
		}
	}
	else
	{
		if (!superuser())
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					 errmsg("permission denied.")));
		}
	}
}
