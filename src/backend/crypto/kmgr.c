/*-------------------------------------------------------------------------
 *
 * kmgr.c
 *	 Cluster file encryption routines
 *
 * Cluster file encryption is enabled if user requests it during initdb.
 * During bootstrap, we generate data encryption keys, wrap them with the
 * cluster-level key, and store them into each file located at KMGR_DIR.
 * During startup, we decrypt all internal keys and load them to the shared
 * memory.  Internal keys in the shared memory are read-only.  The wrapping
 * and unwrapping key routines require the OpenSSL library.
 *
 * Portions Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  src/backend/crypto/kmgr.c
 *
 * add the file for requirement "SRS-SECURITY-TDE"
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include <sys/stat.h>
#include <unistd.h>

#include "funcapi.h"
#include "miscadmin.h"
#include "pgstat.h"

#include "access/xlog.h"
#include "common/file_perm.h"
#include "common/hex.h"
#include "common/kmgr_utils.h"
#include "common/sha2.h"
#include "access/xlog.h"
#include "common/controldata_utils.h"
#include "crypto/kmgr.h"
#include "postmaster/postmaster.h"
#include "storage/copydir.h"
#include "storage/fd.h"
#include "storage/ipc.h"
#include "storage/shmem.h"
#include "utils/builtins.h"
#include "utils/memutils.h"
#include "utils/timestamp.h"

#ifdef USE_SWSDS
#include <swsds.h>
#endif

/* Struct stores file encryption keys in plaintext format */
typedef struct KmgrShmemData
{
	CryptoKey	intlKeys[KMGR_NUM_DATA_KEYS];
} KmgrShmemData;
static KmgrShmemData *KmgrShmem;

/* GUC variables */
char	   *cluster_key_command = NULL;

CryptoKey	bootstrap_keys[KMGR_NUM_DATA_KEYS];

extern char *bootstrap_old_key_datadir;
extern int	bootstrap_file_encryption_method;

#ifdef USE_SWSDS
#define HSM_KWP_ALGORITHM_ID	SGD_SMS4_ECB
#else
#define HSM_KWP_ALGORITHM_ID 0
#endif

static void InitializeKmgr(void);
static void bzeroKmgrKeys(int status, Datum arg);
static void KmgrWriteCryptoKeys(const char *dir, unsigned char **keys, int *key_lens);
static CryptoKey *generate_crypto_key(int len);

/*
 * This function must be called ONCE during initdb.  It creates the DEK
 * files wrapped with the KEK supplied by kmgr_run_cluster_key_command().
 * There is also an option for the keys to be copied from another cluster.
 */
void
BootStrapKmgr(void)
{
	char			live_path[MAXPGPATH];
	unsigned char  *keys_wrap[KMGR_NUM_DATA_KEYS];
	int				key_lens[KMGR_NUM_DATA_KEYS];
	char			cluster_key_hex[ALLOC_KMGR_CLUSTER_KEY_LEN];
	int				cluster_key_hex_len;
	unsigned char 	cluster_key[KMGR_CLUSTER_KEY_LEN];

	if (!FileEncryptionEnabled)
		return;

#ifndef USE_OPENSSL
	ereport(ERROR,
			(errcode(ERRCODE_CONFIG_FILE_ERROR),
			 (errmsg("cluster file encryption is not supported because OpenSSL is not supported by this build"),
			  errhint("Compile with --with-openssl to use this feature."))));
#endif

	/*
	 * There are too many optimizations for wal_level=minimal that don't set
	 * LSNs for permanent tables, so just disallow it.
	 */
	if (!XLogIsNeeded())
		ereport(ERROR,
				(errcode(ERRCODE_CONFIG_FILE_ERROR),
				 (errmsg("cluster file encryption is not supported with a wal_level of \"minimal\""))));

	snprintf(live_path, sizeof(live_path), "%s/%s", DataDir, LIVE_KMGR_DIR);

	/*
	 * Copy cluster file encryption keys from an old cluster? This is useful
	 * for pg_upgrade upgrades where the copied database files are already
	 * encrypted using the old cluster's DEK keys.
	 */
	if (bootstrap_old_key_datadir != NULL)
	{
		char		old_key_dir[MAXPGPATH];

		snprintf(old_key_dir, sizeof(old_key_dir), "%s/%s",
				 bootstrap_old_key_datadir, LIVE_KMGR_DIR);
		copydir(old_key_dir, LIVE_KMGR_DIR, true);

		/* To make the customers happy, provide several copies of the key files. */
		for (int i = 1; i <= NUM_KEY_COPIES; i++)
		{
			char		path[MAXPGPATH];
			pg_snprintf(path, 1024, "%s_bak%d", LIVE_KMGR_DIR, i);
			copydir(pstrdup(LIVE_KMGR_DIR), path, true);
		}
	}
	/* create an empty directory */
	else
	{
		if (mkdir(LIVE_KMGR_DIR, pg_dir_create_mode) < 0)
			ereport(ERROR,
					(errcode_for_file_access(),
					 errmsg("could not create cluster file encryption directory \"%s\": %m",
							LIVE_KMGR_DIR)));
	}

	/*
	 * Get key encryption key (KEK) from the cluster_key command.  The cluster
	 * key command might need to check for the existence of files in the live
	 * directory, e.g., PIV, so run this _after_ copying the directory in
	 * place.
	 */
	cluster_key_hex_len = kmgr_run_cluster_key_command(cluster_key_command,
													   cluster_key_hex,
													   ALLOC_KMGR_CLUSTER_KEY_LEN,
													   live_path, terminal_fd);

	/* decode supplied hex */
	if (!KMGR_USE_HSM_MODE && pg_hex_decode(cluster_key_hex, cluster_key_hex_len,
					  (char *) cluster_key, KMGR_CLUSTER_KEY_LEN) !=
		KMGR_CLUSTER_KEY_LEN)
	{
		ereport(ERROR,
				(errmsg("cluster key must be %d hexadecimal characters",
						KMGR_CLUSTER_KEY_LEN * 2)));
	}

	/* We are not in copy mode?  Generate new cluster file encryption keys. */
	if (bootstrap_old_key_datadir == NULL)
	{
		unsigned char *bootstrap_keys_wrap[KMGR_NUM_DATA_KEYS];
		int	key_lens[KMGR_NUM_DATA_KEYS];
		int	encryption_key_len_bit = encryption_methods[bootstrap_file_encryption_method].bit_length;

		if (!KMGR_USE_HSM_MODE)
		{
			PgCipherCtx *cluster_key_ctx;

			/* Create KEK encryption context */
			cluster_key_ctx = pg_cipher_ctx_create(PG_CIPHER_AES_KWP, cluster_key,
												KMGR_CLUSTER_KEY_LEN, true);
			if (!cluster_key_ctx || !cluster_key_ctx->ctx)
				elog(ERROR, "could not initialize encryption context");

			/* Wrap data encryption keys (DEK) using the key encryption key (KEK) */
			for (int id = 0; id < KMGR_NUM_DATA_KEYS; id++)
			{
				CryptoKey  *key;

				/* generate a DEK */
				key = generate_crypto_key(encryption_key_len_bit / 8);
				bootstrap_keys_wrap[id] = palloc0(sizeof(CryptoKey) + pg_cipher_blocksize(cluster_key_ctx));

				/* wrap DEK with KEK */
				if (!kmgr_wrap_data_key(cluster_key_ctx, key, bootstrap_keys_wrap[id], &(key_lens[id])))
				{
					pg_cipher_ctx_free(cluster_key_ctx);
					elog(ERROR, "failed to wrap data encryption key");
				}

				/* remove DEK from memory */
				explicit_bzero(key, sizeof(CryptoKey));
			}

			pg_cipher_ctx_free(cluster_key_ctx);
		}
		else
		{
			/* Use HSM to generate keys */
			for (int id = 0; id < KMGR_NUM_DATA_KEYS; id++)
			{
				CryptoKey *key;
				bootstrap_keys_wrap[id] = palloc0(sizeof(CryptoKey));
				key_lens[id] = sizeof(CryptoKey);
				key = (CryptoKey*)bootstrap_keys_wrap[id];
				key->kAlgId = HSM_KWP_ALGORITHM_ID;
				hsm_generate_crypto_key(hsm_get_kek_id(), encryption_key_len_bit, key);
			}
		}

		/* Write data encryption keys to the disk */
		KmgrWriteCryptoKeys(LIVE_KMGR_DIR, bootstrap_keys_wrap, key_lens);
	}

	/*
	 * We are either decrypting keys we copied from an old cluster, or
	 * decrypting keys we just wrote above --- either way, we decrypt them
	 * here and store them in a file-scoped variable for use in later
	 * encrypting during bootstrap mode.
	 */

	/* Get the crypto keys from the live directory */
	kmgr_read_wrapped_data_keys(LIVE_KMGR_DIR, keys_wrap, key_lens);

	if (!KMGR_USE_HSM_MODE)
	{
		if (!kmgr_verify_cluster_key(cluster_key, keys_wrap, key_lens, bootstrap_keys))
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("supplied cluster key does not match expected cluster_key")));
	}
	else
	{
		hsm_verify_cluster_key(keys_wrap, key_lens, bootstrap_keys);
	}

	/* bzero DEK on exit */
	on_proc_exit(bzeroKmgrKeys, 0);

	/* bzero KEK */
	explicit_bzero(cluster_key_hex, cluster_key_hex_len);
	explicit_bzero(cluster_key, KMGR_CLUSTER_KEY_LEN);
	close_terminal_fd(terminal_fd);
}

/* Report shared-memory space needed by KmgrShmem */
Size
KmgrShmemSize(void)
{
	if (!FileEncryptionEnabled)
		return 0;

	return MAXALIGN(sizeof(KmgrShmemData));
}

/* Allocate and initialize key manager memory */
void
KmgrShmemInit(void)
{
	bool		found;

	if (!FileEncryptionEnabled)
		return;

	KmgrShmem = (KmgrShmemData *) ShmemInitStruct("File encryption key manager",
												  KmgrShmemSize(), &found);

	/*
	 * The cluster key in shared memory shall be reset once 
	 * every time the shared memory is reinitialized.
	 * However, this shuold not be invoked during bootstrap (it will be set by BootStrapKmgr)
	 */
	if (!IsUnderPostmaster && !IsBootstrapProcessingMode())
	{
		InitializeKmgr();
	}

#ifndef WIN32
	/* bzero DEK on exit */
	on_shmem_exit(bzeroKmgrKeys, 0);
#endif
}

/*
 * Get cluster key and verify it, then get the data encryption keys.
 * This function is called by postmaster at startup time.
 */
static void
InitializeKmgr(void)
{
	unsigned char  *keys_wrap[KMGR_NUM_DATA_KEYS];
	int				key_lens[KMGR_NUM_DATA_KEYS];
	char			cluster_key_hex[ALLOC_KMGR_CLUSTER_KEY_LEN];
	int				cluster_key_hex_len;
	struct stat 	buffer;
	char			live_path[MAXPGPATH];
	unsigned char 	cluster_key[KMGR_CLUSTER_KEY_LEN];

	if (!FileEncryptionEnabled)
		return;

#ifndef USE_OPENSSL
	ereport(ERROR,
			(errcode(ERRCODE_CONFIG_FILE_ERROR),
			 (errmsg("cluster file encryption is not supported because OpenSSL is not supported by this build"),
			  errhint("Compile with --with-openssl to use this feature."))));
#endif

	/*
	 * There are too many optimizations for wal_level=minimal that don't set
	 * LSNs for permanent tables, so just disallow it.
	 */
	if (!XLogIsNeeded())
		ereport(ERROR,
				(errcode(ERRCODE_CONFIG_FILE_ERROR),
				 (errmsg("cluster file encryption is not supported with a wal_level of \"minimal\""))));

	elog(DEBUG1, "starting up cluster file encryption manager");

	if (stat(KMGR_DIR, &buffer) != 0 || !S_ISDIR(buffer.st_mode))
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 (errmsg("cluster file encryption directory %s is missing", KMGR_DIR))));

	if (stat(KMGR_DIR_PID, &buffer) == 0)
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 (errmsg("cluster had a pg_alterckey failure that needs repair or pg_alterckey is running"),
				  errhint("Run pg_alterckey --repair or wait for it to complete."))));

	/*
	 * We want OLD deleted since it allows access to the data encryption keys
	 * using the old cluster key.  If NEW exists, it means either the new
	 * directory is partly written, or NEW wasn't renamed to LIVE --- in
	 * either case, it needs to be repaired.  See src/bin/pg_alterckey/README
	 * for more details.
	 */
	if (stat(OLD_KMGR_DIR, &buffer) == 0 || stat(NEW_KMGR_DIR, &buffer) == 0)
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 (errmsg("cluster had a pg_alterckey failure that needs repair"),
				  errhint("Run pg_alterckey --repair."))));

	/* If OLD, NEW, and LIVE do not exist, there is a serious problem. */
	if (stat(LIVE_KMGR_DIR, &buffer) != 0 || !S_ISDIR(buffer.st_mode))
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 (errmsg("cluster has no data encryption keys"))));

	/* Get the cluster key (KEK) */
	snprintf(live_path, sizeof(live_path), "%s/%s", DataDir, LIVE_KMGR_DIR);
	cluster_key_hex_len = kmgr_run_cluster_key_command(cluster_key_command,
													   cluster_key_hex,
													   ALLOC_KMGR_CLUSTER_KEY_LEN,
													   live_path, terminal_fd);

	/* decode supplied hex */
	if (!KMGR_USE_HSM_MODE && pg_hex_decode(cluster_key_hex, cluster_key_hex_len,
					  (char *) cluster_key, KMGR_CLUSTER_KEY_LEN) !=
		KMGR_CLUSTER_KEY_LEN)
		ereport(ERROR,
				(errmsg("cluster key must be %d hexadecimal characters",
						KMGR_CLUSTER_KEY_LEN * 2)));

	/* Load wrapped DEKs from their files into an array */
	kmgr_read_wrapped_data_keys(LIVE_KMGR_DIR, keys_wrap, key_lens);

	if (!KMGR_USE_HSM_MODE)
	{
		/*
		 * Verify cluster key and store the unwrapped data encryption keys in
		 * shared memory.
		 */
		if (!kmgr_verify_cluster_key(cluster_key, keys_wrap, key_lens, KmgrShmem->intlKeys))
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("supplied cluster key does not match expected cluster key")));
		
		/* Check that retrieved key lengths match controldata length. */
		for (int id = 0; id < KMGR_NUM_DATA_KEYS; id++)
		{
			if (KmgrShmem->intlKeys[id].klen * 8 !=
				encryption_methods[GetFileEncryptionMethod()].bit_length)
			{
				char		path[MAXPGPATH];

				CryptoKeyFilePath(path, DataDir, id);

				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						errmsg("data encryption key %s of length %d does not match controldata key length %d",
								path, KmgrShmem->intlKeys[id].klen * 8,
								encryption_methods[GetFileEncryptionMethod()].bit_length)));
			}
		}
	}
	else
	{
		hsm_verify_cluster_key(keys_wrap, key_lens, KmgrShmem->intlKeys);
	}

	/* Update backup keys. */
	for (int i = 1; i <= NUM_KEY_COPIES; i++)
	{
		char		path[MAXPGPATH];
		pg_snprintf(path, 1024, "%s_bak%d", LIVE_KMGR_DIR, i);
		rmtree(path, true);
		copydir(pstrdup(LIVE_KMGR_DIR), path, true);
	}

	/* bzero KEK */
	explicit_bzero(cluster_key_hex, cluster_key_hex_len);
	explicit_bzero(cluster_key, KMGR_CLUSTER_KEY_LEN);
	close_terminal_fd(terminal_fd);
}

static void
bzeroKmgrKeys(int status, Datum arg)
{
	if (IsBootstrapProcessingMode())
		explicit_bzero(bootstrap_keys, sizeof(bootstrap_keys));
	else
		explicit_bzero(KmgrShmem->intlKeys, sizeof(KmgrShmem->intlKeys));
}

/* return requested DEK */
const CryptoKey *
KmgrGetKey(int id)
{
	Assert(id < KMGR_NUM_DATA_KEYS);

	return (const CryptoKey *) (IsBootstrapProcessingMode() ?
								&(bootstrap_keys[id]) : &(KmgrShmem->intlKeys[id]));
}

/* Generate a DEK inside a CryptoKey */
static CryptoKey *
generate_crypto_key(int len)
{
	CryptoKey  *newkey;

	Assert(len <= KMGR_MAX_KEY_LEN);
	newkey = (CryptoKey *) palloc0(sizeof(CryptoKey));
	newkey->klen = len;
	newkey->kAlgId = PG_CIPHER_AES_KWP;

	if (!pg_strong_random(newkey->key, len))
		elog(ERROR, "failed to generate new file encryption key");

	return newkey;
}

/*
 * Write the DEKs to the disk.
 */
static void
KmgrWriteCryptoKeys(const char *dir, unsigned char **keys, int *key_lens)
{
	elog(DEBUG2, "writing data encryption keys wrapped using the cluster key");

	for (int i = 0; i < KMGR_NUM_DATA_KEYS; i++)
	{
		int			fd;
		char		path[MAXPGPATH];

		CryptoKeyFilePath(path, dir, i);

		if ((fd = BasicOpenFile(path, O_RDWR | O_CREAT | O_EXCL | PG_BINARY)) < 0)
			ereport(ERROR,
					(errcode_for_file_access(),
					 errmsg("could not open file \"%s\": %m",
							path)));

		errno = 0;
		pgstat_report_wait_start(WAIT_EVENT_KEY_FILE_WRITE);
		if (write(fd, keys[i], key_lens[i]) != key_lens[i])
		{
			/* if write didn't set errno, assume problem is no disk space */
			if (errno == 0)
				errno = ENOSPC;

			ereport(ERROR,
					(errcode_for_file_access(),
					 errmsg("could not write file \"%s\": %m",
							path)));
		}
		pgstat_report_wait_end();

		pgstat_report_wait_start(WAIT_EVENT_KEY_FILE_SYNC);
		if (pg_fsync(fd) != 0)
			ereport(PANIC,
					(errcode_for_file_access(),
					 errmsg("could not fsync file \"%s\": %m",
							path)));
		pgstat_report_wait_end();

		if (close(fd) != 0)
			ereport(ERROR,
					(errcode_for_file_access(),
					 errmsg("could not close file \"%s\": %m",
							path)));
	}

	/* To make the customers happy, provide several copies of the key files. */
	for (int i = 1; i <= NUM_KEY_COPIES; i++)
	{
		char		path[MAXPGPATH];
		pg_snprintf(path, 1024, "%s_bak%d", dir, i);
		copydir(pstrdup(dir), path, true);
	}
}

/*
 * Show all encryption keys infomation
 */
Datum
show_all_encryption_keys(PG_FUNCTION_ARGS)
{
#define	NUM_PG_ENCRYPTION_KEY_ATTS	4
	ReturnSetInfo  *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	TupleDesc		tupdesc;
	Tuplestorestate *tupstore;
	int				seqno;
	MemoryContext	per_query_ctx;
	MemoryContext	oldcontext;

	/* Check to see if caller supports us returning a tuplestore */
	if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("set-valued function called in context that cannot accept a set")));
	if (!(rsinfo->allowedModes & SFRM_Materialize))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("materialize mode required, but it is not allowed in this context")));

	/* Switch into long-lived context to construct returned data structures */
	per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
	oldcontext = MemoryContextSwitchTo(per_query_ctx);

	/* Build a tuple descriptor for our result type */
	tupdesc = CreateTemplateTupleDesc(NUM_PG_ENCRYPTION_KEY_ATTS);
	TupleDescInitEntry(tupdesc, (AttrNumber) 1, "keyid",
					   INT4OID, -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 2, "name",
					   TEXTOID, -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 3, "method",
					   TEXTOID, -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 4, "last_rotation_time",
					   TEXTOID, -1, 0);

	/* Build a tuplestore to return our results in */
	tupstore = tuplestore_begin_heap(true, false, work_mem);
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupstore;
	rsinfo->setDesc = tupdesc;

	/* The rest can be done in short-lived context */
	MemoryContextSwitchTo(oldcontext);

	/* Process the results and create a tuplestore */
	for (seqno = 0; seqno < KMGR_NUM_DATA_KEYS; seqno++)
	{
		char		path[MAXPGPATH];
		int			fd;
		int			r;
		struct stat st;
		time_t		birthtime;
		int			key_len;
		char	   *key_p;
		Datum		values[NUM_PG_ENCRYPTION_KEY_ATTS];
		bool		nulls[NUM_PG_ENCRYPTION_KEY_ATTS];

		CryptoKeyFilePath(path, LIVE_KMGR_DIR, seqno);

		if ((fd = OpenTransientFile(path, O_RDONLY | PG_BINARY)) == -1)
			ereport(ERROR,
					(errcode_for_file_access(),
					 errmsg("could not open file \"%s\" for reading: %m",
							path)));
		else
			if (fstat(fd, &st))
				ereport(ERROR,
						(errcode_for_file_access(),
						 errmsg("could not stat file \"%s\": %m",
								path)));

		key_len = st.st_size;
		birthtime = st.st_mtime;
		pgstat_report_wait_start(WAIT_EVENT_KEY_FILE_READ);
		key_p = palloc0(key_len);

		/* Get key bytes */
		r = read(fd, key_p, key_len);
		if (r != key_len)
		{
			if (r < 0)
			{
				ereport(ERROR,
						(errcode_for_file_access(),
						 errmsg("could not read file \"%s\": %m", path)));
			}
			else
			{
				ereport(ERROR,
						(errcode(ERRCODE_DATA_CORRUPTED),
						 errmsg("could not read file \"%s\": read %d of %u",
								path, r, key_len)));
			}
		}

		pgstat_report_wait_end();

		if (CloseTransientFile(fd) != 0)
			ereport(ERROR,
					(errcode_for_file_access(),
					 errmsg("could not close file \"%s\": %m",
							path)));

		memset(values, 0, sizeof(values));
		memset(nulls, 0, sizeof(nulls));

		values[0] = Int32GetDatum(seqno);
		values[1] = PointerGetDatum(cstring_to_text(wkey_filenames[seqno]));
		values[2] = PointerGetDatum(cstring_to_text(encryption_methods[GetFileEncryptionMethod()].name));
		values[3] = PointerGetDatum(cstring_to_text(timestamptz_to_str(time_t_to_timestamptz(birthtime))));

		/* show row into tuplestore */
		tuplestore_putvalues(tupstore, tupdesc, values, nulls);
	}

	tuplestore_donestoring(tupstore);

	return (Datum) 0;
}
