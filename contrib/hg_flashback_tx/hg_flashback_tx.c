#include "postgres.h"
#include <limits.h>
#include <string.h>
#include "fmgr.h"
#include "funcapi.h"
#include "libpq/pqsignal.h"
#include "miscadmin.h"
#include "postmaster/bgworker.h"
#include "executor/spi.h"
#include "access/xact.h"
#include "access/transam.h"
#include "catalog/pg_type.h"
#include "catalog/pg_extension.h"
#include "catalog/pg_class.h"
#include "catalog/pg_attribute.h"
#include "catalog/namespace.h"
#include "catalog/indexing.h"
#include "common/jsonapi.h"
#include "nodes/pg_list.h"
#include "mb/pg_wchar.h"
#include "parser/parse_relation.h"
#include "storage/ipc.h"
#include "access/tupdesc.h"
#include "utils/elog.h"
#include "utils/guc.h"
#include "utils/builtins.h"
#include "utils/hsearch.h"
#include "utils/memutils.h"
#include "utils/snapmgr.h"
#include "utils/timestamp.h"
#include "utils/resowner.h"
#include "utils/syscache.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"
#include "utils/jsonfuncs.h"
#include "tcop/utility.h"
#include "access/genam.h"
#include "access/table.h"
#include "access/xlog.h"
#include "access/xlogutils.h"
#include "replication/decode.h"
#include "replication/logical.h"
#include "replication/slot.h"
#include "utils/inval.h"
#include "utils/tuplestore.h"
#include "executor/tuptable.h"

PG_MODULE_MAGIC;

void _PG_init(void);
void _PG_fini(void);

/* GUCs */
static bool enable_flashback_transaction_query = false;
static bool enable_flashback_transaction_query_auto_purge = false;
static int worker_poll_interval = 1;
static int undo_retention = 0;
static char *flashback_databases = "";
static bool enable_auto_replica_identity = false;

static volatile sig_atomic_t got_sigterm = false;
static volatile sig_atomic_t got_sighup = false;

static char current_db_name[NAMEDATALEN];
static bool extension_created = false;
static LogicalDecodingContext *decode_ctx = NULL;
static bool decoding_initialized = false;
static MemoryContext decode_top_mcxt = NULL;
static MemoryContext decode_cycle_mcxt = NULL;

/* ProcessUtility_hook */
static ProcessUtility_hook_type prev_ProcessUtility_hook = NULL;
static void hg_flashback_tx_utility_hook(PlannedStmt *pstmt,
									 const char *queryString,
									 bool readOnlyTree,
									 ProcessUtilityContext context,
									 ParamListInfo params,
									 QueryEnvironment *queryEnv,
									 DestReceiver *dest,
									 QueryCompletion *qc);

PGDLLEXPORT void hg_flashback_tx_worker_main(Datum main_arg);

/* Background worker and logical decoding helper functions. */
static void hg_flashback_tx_ensure_slot(void);
static void hg_flashback_tx_purge_old_rows(void);
static void hg_flashback_tx_init_decoding_context(void);
static void hg_flashback_tx_cleanup_decoding_context(void);
static void hg_flashback_tx_consume_changes_persistent(void);
static void hg_flashback_tx_decode_prepare_write(LogicalDecodingContext *ctx,
								 XLogRecPtr lsn, TransactionId xid, bool last_write);
static void hg_flashback_tx_decode_do_write(LogicalDecodingContext *ctx,
							 XLogRecPtr lsn, TransactionId xid, bool last_write);
static void hg_flashback_tx_decode_update_progress(LogicalDecodingContext *ctx,
								   XLogRecPtr lsn, TransactionId xid);
static List *hg_flashback_tx_process_raw_messages(void);
static uint64 hg_flashback_tx_flush_committed_txns(List *committed_xids);
static void hg_flashback_tx_free_entries(Tuplestorestate *entries);
static void hg_flashback_tx_ensure_pending_entry_tupdesc(void);
static void hg_flashback_tx_build_undo_column_lists_from_pairs(StringInfo colbuf, StringInfo valbuf,
											 List *pairs, bool for_update);
static bool hg_flashback_tx_parse_json_message(const char *json, char **op, char **nsp,
								 char **relname, char **table_owner,
								 TimestampTz *commit_time, List **locator_pairs,
								 List **column_pairs);

typedef struct HgFlashbackSeqEntry
{
	TransactionId xid;
	uint64		seq;
} HgFlashbackSeqEntry;

typedef struct HgFlashbackRawMsg
{
	XLogRecPtr	lsn;
	TransactionId xid;
	bool		last_write;
	char	   *data;
} HgFlashbackRawMsg;

typedef struct HgFlashbackNameVal
{
	char	   *name;
	char	   *val;
} HgFlashbackNameVal;

typedef enum HgFlashbackJsonArrayCtx
{
	HG_FB_JSON_ARR_NONE = 0,
	HG_FB_JSON_ARR_LOCATOR,
	HG_FB_JSON_ARR_COLS
} HgFlashbackJsonArrayCtx;

typedef struct HgFlashbackJsonState
{
	char	   *op;
	char	   *nsp;
	char	   *relname;
	char	   *table_owner;
	TimestampTz	commit_time;
	List	   *locator_pairs;
	List	   *column_pairs;
	char	   *current_key;
	char	   *pair_name;
	char	   *pair_val;
	bool		in_pair_object;
	bool		malformed;
	HgFlashbackJsonArrayCtx array_ctx;
} HgFlashbackJsonState;

typedef struct HgFlashbackCallbackData
{
	List	   *raw_msgs;
	uint64		write_calls;
} HgFlashbackCallbackData;

typedef struct HgFlashbackPendingTxn
{
	TransactionId xid;
	Tuplestorestate *entries;
	uint64		entry_count;
	bool		seen_begin;
	bool		ready_to_flush;
} HgFlashbackPendingTxn;

static HgFlashbackCallbackData *decode_cb_data = NULL;
static MemoryContext pending_txn_mcxt = NULL;
static HTAB	   *pending_txn_hash = NULL;
static TupleDesc pending_entry_tupdesc = NULL;
static ResourceOwner pending_tuplestore_resowner = NULL;

void
_PG_init(void)
{
	BackgroundWorker worker;

	/*
	 * GUCs for toggles and retention policy.
	 * enable_flashback_transaction_query: gate all writes into
	 * flashback_transaction_query to avoid extra overhead when disabled.
	 * enable_flashback_transaction_query_auto_purge: allow background cleanup
	 * to prevent unbounded growth of the metadata table.
	 * undo_retention: retention window in seconds, applied against
	 * flashback_transaction_query.commit_timestamp.
	 */
	DefineCustomBoolVariable("hg_flashback_tx.enable_flashback_transaction_query",
							 "Enable flashback transaction query.",
							 NULL,
							 &enable_flashback_transaction_query,
							 false,
							 PGC_SIGHUP,
							 0,
							 NULL,
							 NULL,
							 NULL);

	DefineCustomBoolVariable("hg_flashback_tx.enable_flashback_transaction_query_auto_purge",
							 "Enable auto purge for flashback transaction query.",
							 NULL,
							 &enable_flashback_transaction_query_auto_purge,
							 false,
							 PGC_SIGHUP,
							 0,
							 NULL,
							 NULL,
							 NULL);

	DefineCustomIntVariable("hg_flashback_tx.worker_poll_interval",
							"Background worker polling interval, in seconds.",
							NULL,
							&worker_poll_interval,
							1,
							1,
							INT_MAX / 1000000,
							PGC_SIGHUP,
							GUC_UNIT_S,
							NULL,
							NULL,
							NULL);

	DefineCustomIntVariable("hg_flashback_tx.undo_retention",
							"Minimum time to retain undo data, in seconds.",
							NULL,
							&undo_retention,
							0,
							0,
							INT_MAX,
							PGC_SIGHUP,
							GUC_UNIT_S,
							NULL,
							NULL,
							NULL);

	DefineCustomStringVariable("hg_flashback_tx.databases",
							   "Comma-separated list of databases to monitor for flashback.",
							   NULL,
							   &flashback_databases,
							   "postgres",
							   PGC_SIGHUP,
							   GUC_LIST_INPUT,
							   NULL,
							   NULL,
							   NULL);

	DefineCustomBoolVariable("hg_flashback_tx.auto_replica_identity",
							 "Automatically set REPLICA IDENTITY FULL for all tables.",
							 NULL,
							 &enable_auto_replica_identity,
							 false,
							 PGC_SIGHUP,
							 0,
							 NULL,
							 NULL,
							 NULL);

	prev_ProcessUtility_hook = ProcessUtility_hook;
	ProcessUtility_hook = hg_flashback_tx_utility_hook;

	if (!process_shared_preload_libraries_in_progress)
		return;

	/*
	 * Register background worker for each database (requires shared_preload_libraries).
	 * If the library was loaded dynamically, we skip registration to avoid
	 * creating workers outside postmaster startup.
	 */
	if (flashback_databases != NULL && strlen(flashback_databases) > 0)
	{
		char	   *db_list = pstrdup(flashback_databases);
		char	   *token;
		int			db_index = 0;

		token = strtok(db_list, ", \t");
		while (token != NULL)
		{
			while (*token == ' ')
				token++;

			memset(&worker, 0, sizeof(worker));
			snprintf(worker.bgw_name, BGW_MAXLEN, "hg_flashback_tx worker (%s)", token);
			snprintf(worker.bgw_type, BGW_MAXLEN, "hg_flashback_tx");
			worker.bgw_flags = BGWORKER_SHMEM_ACCESS |
				BGWORKER_BACKEND_DATABASE_CONNECTION;
			worker.bgw_start_time = BgWorkerStart_RecoveryFinished;
			worker.bgw_restart_time = BGW_NEVER_RESTART;
			snprintf(worker.bgw_library_name, BGW_MAXLEN, "hg_flashback_tx");
			snprintf(worker.bgw_function_name, BGW_MAXLEN, "hg_flashback_tx_worker_main");
			worker.bgw_main_arg = Int32GetDatum(db_index);
			worker.bgw_notify_pid = 0;

			RegisterBackgroundWorker(&worker);

			token = strtok(NULL, ", \t");
			db_index++;
		}
	}
}

void
_PG_fini(void)
{
	ProcessUtility_hook = prev_ProcessUtility_hook;
}

/* Signal handlers for the background worker. */
static void
hg_flashback_tx_sighup_handler(SIGNAL_ARGS)
{
	int			save_errno = errno;

	got_sighup = true;
	errno = save_errno;
}

static void
hg_flashback_tx_sigterm_handler(SIGNAL_ARGS)
{
	int			save_errno = errno;

	got_sigterm = true;
	errno = save_errno;
}

/* Background worker main loop. */
void
hg_flashback_tx_worker_main(Datum main_arg)
{
	int			db_index = DatumGetInt32(main_arg);
	char	   *target_db = NULL;
	char	   *db_list;
	char	   *token;
	int			current_index = 0;

	pqsignal(SIGHUP, hg_flashback_tx_sighup_handler);
	pqsignal(SIGTERM, hg_flashback_tx_sigterm_handler);
	BackgroundWorkerUnblockSignals();

	/*
	 * Get the target database from the GUC list using the index.
	 */
	if (flashback_databases != NULL && strlen(flashback_databases) > 0)
	{
		db_list = pstrdup(flashback_databases);
		token = strtok(db_list, ", \t");
		while (token != NULL)
		{
			while (*token == ' ')
				token++;

			if (current_index == db_index)
			{
				target_db = pstrdup(token);
				break;
			}
			token = strtok(NULL, ", \t");
			current_index++;
		}
	}

	if (target_db == NULL)
	{
		elog(LOG, "hg_flashback_tx worker: database not found at index %d, flashback_databases='%s'", db_index, flashback_databases ? flashback_databases : "(null)");
		return;
	}

	elog(LOG, "hg_flashback_tx worker connecting to database: %s", target_db);

	/*
	 * Connect as a backend to run SPI queries.
	 * This establishes a database connection in a normal catalog
	 * context and can access pg_replication_slots.
	 */
	BackgroundWorkerInitializeConnection(target_db, NULL, 0);

	strlcpy(current_db_name, target_db, sizeof(current_db_name));

	if (!extension_created)
	{
		StartTransactionCommand();
		if (SPI_connect() != SPI_OK_CONNECT)
			elog(ERROR, "SPI_connect failed");

		PushActiveSnapshot(GetTransactionSnapshot());

		if (SPI_execute(
				"SELECT 1 FROM pg_extension WHERE extname = 'hg_flashback_tx' LIMIT 1",
				true, 0) != SPI_OK_SELECT)
			elog(ERROR, "SPI_execute failed for extension check");

		if (SPI_processed == 0)
		{
			PopActiveSnapshot();
			SPI_finish();
			CommitTransactionCommand();

			StartTransactionCommand();
			if (SPI_connect() != SPI_OK_CONNECT)
				elog(ERROR, "SPI_connect failed");

			PushActiveSnapshot(GetTransactionSnapshot());

			if (SPI_execute(
					"CREATE EXTENSION IF NOT EXISTS hg_flashback_tx",
					false, 0) != SPI_OK_UTILITY)
				elog(ERROR, "Failed to create hg_flashback_tx extension");

			extension_created = true;
			elog(LOG, "hg_flashback_tx extension created in database \"%s\"", target_db);
		}
		else
		{
			extension_created = true;
		}

		PopActiveSnapshot();
		SPI_finish();
		CommitTransactionCommand();

		ProcessConfigFile(PGC_SIGHUP);
	}

	elog(LOG, "hg_flashback_tx worker started for database: %s", target_db);

	while (!got_sigterm)
	{
		if (got_sighup)
		{
			ProcessConfigFile(PGC_SIGHUP);
			got_sighup = false;
		}

		/*
		 * Ensure slot and optionally purge before consuming changes.
		 * The ordering is deliberate:
		 * 1) Ensure slot exists so pg_logical_slot_get_changes can work.
		 * 2) Purge old rows first to bound table size before new inserts.
		 * 3) Consume and record new changes.
		 */
			PG_TRY();
			{
				hg_flashback_tx_ensure_slot();
				if (enable_flashback_transaction_query_auto_purge)
					hg_flashback_tx_purge_old_rows();
				hg_flashback_tx_consume_changes_persistent();
			}
			PG_CATCH();
			{
				ErrorData  *edata;

				AbortCurrentTransaction();
				if (decoding_initialized)
					hg_flashback_tx_cleanup_decoding_context();
				edata = CopyErrorData();
				FlushErrorState();
				elog(LOG, "hg_flashback_tx worker error: %s", edata->message);
			FreeErrorData(edata);
		}
		PG_END_TRY();

		pg_usleep((long) worker_poll_interval * 1000000L);
	}

	if (decoding_initialized)
		hg_flashback_tx_cleanup_decoding_context();

	elog(LOG, "hg_flashback_tx worker exiting");
}

/* Initialize long-lived decoding and pending-transaction contexts. */
static void
hg_flashback_tx_init_decoding_context(void)
{
	char	   *slot_name;

	if (decoding_initialized)
		return;

	if (decode_top_mcxt == NULL)
		decode_top_mcxt = AllocSetContextCreate(TopMemoryContext,
												"hg_flashback_tx decode top",
												ALLOCSET_DEFAULT_SIZES);
	if (decode_cycle_mcxt == NULL)
		decode_cycle_mcxt = AllocSetContextCreate(decode_top_mcxt,
												  "hg_flashback_tx decode cycle",
												  ALLOCSET_DEFAULT_SIZES);
	if (pending_txn_mcxt == NULL)
		pending_txn_mcxt = AllocSetContextCreate(decode_top_mcxt,
												 "hg_flashback_tx pending txns",
												 ALLOCSET_DEFAULT_SIZES);

	slot_name = psprintf("hg_flashback_tx_slot_%s", current_db_name);
	ReplicationSlotAcquire(slot_name, true);
	pfree(slot_name);

	decode_ctx = CreateDecodingContext(InvalidXLogRecPtr,
									   NIL,
									   false,
									   XL_ROUTINE(.page_read = read_local_xlog_page,
												  .segment_open = wal_segment_open,
												  .segment_close = wal_segment_close),
									   hg_flashback_tx_decode_prepare_write,
									   hg_flashback_tx_decode_do_write,
									   hg_flashback_tx_decode_update_progress);

	if (decode_ctx->options.output_type != OUTPUT_PLUGIN_TEXTUAL_OUTPUT)
		elog(ERROR, "flashback_decoder must use textual output");

	if (XLogRecPtrIsInvalid(MyReplicationSlot->data.restart_lsn))
		elog(ERROR, "replication slot restart_lsn is invalid");

	if (decode_cb_data == NULL)
		decode_cb_data = MemoryContextAllocZero(decode_top_mcxt,
												sizeof(HgFlashbackCallbackData));
	if (pending_txn_hash == NULL)
	{
		HASHCTL		hashctl;

		memset(&hashctl, 0, sizeof(hashctl));
		hashctl.keysize = sizeof(TransactionId);
		hashctl.entrysize = sizeof(HgFlashbackPendingTxn);
		hashctl.hcxt = decode_top_mcxt;
		pending_txn_hash = hash_create("hg_flashback_tx pending txns",
									   128,
									   &hashctl,
									   HASH_ELEM | HASH_BLOBS | HASH_CONTEXT);
	}
	if (pending_tuplestore_resowner == NULL)
		pending_tuplestore_resowner =
			ResourceOwnerCreate(AuxProcessResourceOwner,
								"hg_flashback_tx pending tuplestore");
	decode_ctx->output_writer_private = decode_cb_data;
	XLogBeginRead(decode_ctx->reader, MyReplicationSlot->data.restart_lsn);
	InvalidateSystemCaches();
	DecodingContextFindStartpoint(decode_ctx);
	decoding_initialized = true;
	elog(LOG, "hg_flashback_tx decoding context initialized");
}

static void
hg_flashback_tx_cleanup_decoding_context(void)
{
	HASH_SEQ_STATUS scan;
	HgFlashbackPendingTxn *txn_entry;

	if (pending_txn_hash != NULL)
	{
		hash_seq_init(&scan, pending_txn_hash);
		while ((txn_entry = hash_seq_search(&scan)) != NULL)
		{
			if (txn_entry->entries != NULL)
				hg_flashback_tx_free_entries(txn_entry->entries);
		}
	}

	if (decode_ctx != NULL)
		FreeDecodingContext(decode_ctx);
	decode_ctx = NULL;

	if (MyReplicationSlot != NULL)
		ReplicationSlotRelease();

	if (decode_top_mcxt != NULL)
		MemoryContextDelete(decode_top_mcxt);
	decode_top_mcxt = NULL;
	decode_cycle_mcxt = NULL;
	pending_txn_mcxt = NULL;
	decode_cb_data = NULL;
	pending_txn_hash = NULL;
	pending_entry_tupdesc = NULL;
	if (pending_tuplestore_resowner != NULL)
		ResourceOwnerDelete(pending_tuplestore_resowner);
	pending_tuplestore_resowner = NULL;

	InvalidateSystemCaches();
	decoding_initialized = false;
}

static void
hg_flashback_tx_consume_changes_persistent(void)
{
	/*
	 * Persistent decode loop:
	 * 1) Read WAL records through a long-lived LogicalDecodingContext.
	 * 2) Writer callback enqueues textual plugin output into raw_msgs.
	 * 3) Parse raw messages into per-xid pending entries.
	 * 4) Flush xid groups that have seen COMMIT marker ('C').
	 * 5) Advance slot confirmed_flush after this cycle is fully processed.
	 *
	 * Any failure in this block triggers context teardown in PG_CATCH to avoid
	 * carrying a potentially-corrupted decoding state into the next loop.
	 */
	PG_TRY();
	{
		XLogRecPtr	end_of_wal;
		uint64		processed_records = 0;
		List	   *committed_xids;
		uint64		flushed_entries = 0;

		if (!decoding_initialized)
			hg_flashback_tx_init_decoding_context();

		MemoryContextReset(decode_cycle_mcxt);
		decode_cb_data->raw_msgs = NIL;
		decode_cb_data->write_calls = 0;
		decode_ctx->output_writer_private = decode_cb_data;

		end_of_wal = GetFlushRecPtr();
		while (decode_ctx->reader->EndRecPtr < end_of_wal)
		{
			XLogRecord *record;
			char	   *errm = NULL;

			record = XLogReadRecord(decode_ctx->reader, &errm);
			if (errm != NULL)
				elog(ERROR, "%s", errm);

			if (record != NULL)
			{
				LogicalDecodingProcessRecord(decode_ctx, decode_ctx->reader);
				processed_records++;
			}
			CHECK_FOR_INTERRUPTS();
		}

		StartTransactionCommand();
		committed_xids = hg_flashback_tx_process_raw_messages();
		flushed_entries = hg_flashback_tx_flush_committed_txns(committed_xids);
		CommitTransactionCommand();
		if (flushed_entries > 0)
			elog(DEBUG1, "hg_flashback_tx flushed %llu undo entries",
				 (unsigned long long) flushed_entries);

		if (decode_ctx->reader->EndRecPtr != InvalidXLogRecPtr && processed_records > 0)
		{
			LogicalConfirmReceivedLocation(decode_ctx->reader->EndRecPtr);
			ReplicationSlotMarkDirty();
		}
	}
	PG_CATCH();
	{
		ErrorData  *edata;

		if (IsTransactionState())
			AbortCurrentTransaction();
		edata = CopyErrorData();
		FlushErrorState();
		if (decoding_initialized)
			hg_flashback_tx_cleanup_decoding_context();
		elog(LOG, "hg_flashback_tx recovered from error, context reset: %s",
			 edata->message);
		FreeErrorData(edata);
		return;
	}
	PG_END_TRY();
}

static void
hg_flashback_tx_decode_prepare_write(LogicalDecodingContext *ctx, XLogRecPtr lsn,
								 TransactionId xid, bool last_write)
{
	resetStringInfo(ctx->out);
}

static void
hg_flashback_tx_decode_do_write(LogicalDecodingContext *ctx, XLogRecPtr lsn,
							TransactionId xid, bool last_write)
{
	HgFlashbackCallbackData *cb_data = ctx->output_writer_private;
	HgFlashbackRawMsg *msg;
	MemoryContext oldcxt;

	if (cb_data == NULL || decode_cycle_mcxt == NULL)
		return;

	oldcxt = MemoryContextSwitchTo(decode_cycle_mcxt);
	msg = palloc(sizeof(HgFlashbackRawMsg));
	msg->lsn = lsn;
	msg->xid = xid;
	msg->last_write = last_write;
	msg->data = pstrdup(ctx->out->data);
	cb_data->raw_msgs = lappend(cb_data->raw_msgs, msg);
	cb_data->write_calls++;
	MemoryContextSwitchTo(oldcxt);
}

static void
hg_flashback_tx_decode_update_progress(LogicalDecodingContext *ctx, XLogRecPtr lsn,
								   TransactionId xid)
{
	/* Progress confirmation is handled after message processing in consume loop. */
}

static void
hg_flashback_tx_json_object_start(void *state)
{
	HgFlashbackJsonState *st = state;

	if (st->array_ctx != HG_FB_JSON_ARR_NONE)
	{
		if (st->in_pair_object)
			st->malformed = true;
		st->in_pair_object = true;
		if (st->pair_name)
			pfree(st->pair_name);
		if (st->pair_val)
			pfree(st->pair_val);
		st->pair_name = NULL;
		st->pair_val = NULL;
	}
}

static void
hg_flashback_tx_json_object_end(void *state)
{
	HgFlashbackJsonState *st = state;

	if (st->in_pair_object)
	{
		HgFlashbackNameVal *pair;

		if (st->pair_name == NULL || st->pair_val == NULL)
		{
			st->malformed = true;
			return;
		}

		pair = palloc(sizeof(HgFlashbackNameVal));
		pair->name = st->pair_name;
		pair->val = st->pair_val;
		st->pair_name = NULL;
		st->pair_val = NULL;
		if (st->array_ctx == HG_FB_JSON_ARR_LOCATOR)
			st->locator_pairs = lappend(st->locator_pairs, pair);
		else if (st->array_ctx == HG_FB_JSON_ARR_COLS)
			st->column_pairs = lappend(st->column_pairs, pair);
		else
		{
			st->malformed = true;
			pfree(pair->name);
			pfree(pair->val);
			pfree(pair);
		}
		st->in_pair_object = false;
	}
}

static void
hg_flashback_tx_json_array_start(void *state)
{
	HgFlashbackJsonState *st = state;

	if (st->current_key == NULL)
	{
		st->malformed = true;
		return;
	}

	if (strcmp(st->current_key, "locator") == 0)
		st->array_ctx = HG_FB_JSON_ARR_LOCATOR;
	else if (strcmp(st->current_key, "cols") == 0)
		st->array_ctx = HG_FB_JSON_ARR_COLS;
	else
		st->malformed = true;

	pfree(st->current_key);
	st->current_key = NULL;
}

static void
hg_flashback_tx_json_array_end(void *state)
{
	HgFlashbackJsonState *st = state;

	st->array_ctx = HG_FB_JSON_ARR_NONE;
}

static void
hg_flashback_tx_json_object_field_start(void *state, char *fname, bool isnull)
{
	HgFlashbackJsonState *st = state;

	if (st->current_key)
		pfree(st->current_key);
	st->current_key = fname;
	if (isnull)
		st->malformed = true;
}

static void
hg_flashback_tx_json_scalar(void *state, char *token, JsonTokenType tokentype)
{
	HgFlashbackJsonState *st = state;

	if (st->current_key == NULL)
	{
		st->malformed = true;
		pfree(token);
		return;
	}

	if (st->in_pair_object)
	{
		if (strcmp(st->current_key, "name") == 0)
		{
			if (st->pair_name)
				pfree(st->pair_name);
			st->pair_name = token;
			token = NULL;
		}
		else if (strcmp(st->current_key, "val") == 0)
		{
			if (st->pair_val)
				pfree(st->pair_val);
			st->pair_val = token;
			token = NULL;
		}
		else
			st->malformed = true;
	}
	else
	{
		if (strcmp(st->current_key, "op") == 0)
		{
			if (st->op)
				pfree(st->op);
			st->op = token;
			token = NULL;
		}
		else if (strcmp(st->current_key, "nsp") == 0)
		{
			if (st->nsp)
				pfree(st->nsp);
			st->nsp = token;
			token = NULL;
		}
		else if (strcmp(st->current_key, "rel") == 0)
		{
			if (st->relname)
				pfree(st->relname);
			st->relname = token;
			token = NULL;
		}
		else if (strcmp(st->current_key, "owner") == 0)
		{
			if (st->table_owner)
				pfree(st->table_owner);
			st->table_owner = token;
			token = NULL;
		}
		else if (strcmp(st->current_key, "commit_time") == 0)
		{
			st->commit_time = (TimestampTz) strtoull(token, NULL, 10);
		}
	}

	if (token)
		pfree(token);
	pfree(st->current_key);
	st->current_key = NULL;
}

static bool
hg_flashback_tx_parse_json_message(const char *json, char **op, char **nsp,
								 char **relname, char **table_owner,
								 TimestampTz *commit_time, List **locator_pairs,
								 List **column_pairs)
{
	JsonLexContext *lex;
	JsonSemAction sem;
	JsonParseErrorType err;
	HgFlashbackJsonState st;

	memset(&st, 0, sizeof(st));
	memset(&sem, 0, sizeof(sem));
	sem.semstate = &st;
	sem.object_start = hg_flashback_tx_json_object_start;
	sem.object_end = hg_flashback_tx_json_object_end;
	sem.array_start = hg_flashback_tx_json_array_start;
	sem.array_end = hg_flashback_tx_json_array_end;
	sem.object_field_start = hg_flashback_tx_json_object_field_start;
	sem.scalar = hg_flashback_tx_json_scalar;

	lex = makeJsonLexContextCstringLen((char *) json, strlen(json),
									   GetDatabaseEncoding(), true);
	err = pg_parse_json(lex, &sem);
	if (err != JSON_SUCCESS || st.malformed || st.op == NULL)
		return false;

	*op = st.op;
	*nsp = st.nsp;
	*relname = st.relname;
	*table_owner = st.table_owner;
	*commit_time = st.commit_time;
	*locator_pairs = st.locator_pairs;
	*column_pairs = st.column_pairs;
	return true;
}

static void
hg_flashback_tx_ensure_pending_entry_tupdesc(void)
{
	MemoryContext oldcxt;

	if (pending_entry_tupdesc != NULL)
		return;

	/*
	 * TupleDescInitEntry() uses syscache lookups and therefore requires a valid
	 * CurrentResourceOwner. Caller must ensure we are inside a transaction.
	 */
	oldcxt = MemoryContextSwitchTo(decode_top_mcxt);
	pending_entry_tupdesc = CreateTemplateTupleDesc(6, false);
	TupleDescInitEntry(pending_entry_tupdesc, 1, "operation", TEXTOID, -1, 0);
	TupleDescInitEntry(pending_entry_tupdesc, 2, "table_name", TEXTOID, -1, 0);
	TupleDescInitEntry(pending_entry_tupdesc, 3, "table_owner", TEXTOID, -1, 0);
	TupleDescInitEntry(pending_entry_tupdesc, 4, "row_id", TEXTOID, -1, 0);
	TupleDescInitEntry(pending_entry_tupdesc, 5, "undo_sql", TEXTOID, -1, 0);
	TupleDescInitEntry(pending_entry_tupdesc, 6, "commit_timestamp", TIMESTAMPTZOID, -1, 0);
	BlessTupleDesc(pending_entry_tupdesc);
	MemoryContextSwitchTo(oldcxt);
}

static List *
hg_flashback_tx_process_raw_messages(void)
{
	ListCell   *lc;
	MemoryContext oldcxt;
	uint64		b_cnt = 0;
	uint64		c_cnt = 0;
	uint64		i_cnt = 0;
	uint64		u_cnt = 0;
	uint64		d_cnt = 0;
	uint64		x_cnt = 0;
	uint64		other_cnt = 0;
	List	   *committed_xids = NIL;

	/*
	 * Keep transient parse allocations (tokenized copies, committed_xids list)
	 * in decode_cycle_mcxt. The caller resets this context each worker loop,
	 * so we avoid unbounded growth in TopMemoryContext.
	 */
	oldcxt = MemoryContextSwitchTo(decode_cycle_mcxt);
	hg_flashback_tx_ensure_pending_entry_tupdesc();

	foreach(lc, decode_cb_data->raw_msgs)
	{
		HgFlashbackRawMsg *msg = lfirst(lc);
		char	   *operation;
		char	   *nspname = NULL;
		char	   *relname = NULL;
		char	   *table_owner = NULL;
		TransactionId xid = msg->xid;
		HgFlashbackPendingTxn *txn_entry;
		TimestampTz	commit_time = 0;
		List	   *locator_pairs = NIL;
		List	   *column_pairs = NIL;
		bool		found;

		if (!TransactionIdIsValid(xid) || msg->data == NULL)
			continue;

		if (!hg_flashback_tx_parse_json_message(msg->data, &operation, &nspname,
											 &relname, &table_owner,
											 &commit_time, &locator_pairs,
											 &column_pairs))
		{
			other_cnt++;
			continue;
		}

		txn_entry = hash_search(pending_txn_hash, &xid, HASH_ENTER, &found);
		if (!found)
		{
			/* New xid group: keep entries in pending_txn_mcxt across cycles. */
			MemoryContext txn_oldcxt = MemoryContextSwitchTo(pending_txn_mcxt);
			ResourceOwner oldowner = CurrentResourceOwner;

			txn_entry->xid = xid;
			/*
			 * Pending xid groups can span multiple worker transactions. Bind
			 * tuplestore to a long-lived resource owner and allow interXact temp
			 * files so spilled data remains valid until explicit cleanup.
			 */
			CurrentResourceOwner = pending_tuplestore_resowner;
			txn_entry->entries = tuplestore_begin_heap(false, true, work_mem);
			CurrentResourceOwner = oldowner;
			txn_entry->entry_count = 0;
			txn_entry->seen_begin = false;
			txn_entry->ready_to_flush = false;
			MemoryContextSwitchTo(txn_oldcxt);
		}

		if (strcmp(operation, "B") == 0)
		{
			/* Transaction begin marker emitted by flashback_decoder. */
			b_cnt++;
			txn_entry->seen_begin = true;
		}
		else if (strcmp(operation, "C") == 0)
		{
			/* Transaction commit marker: xid becomes eligible for flush. */
			c_cnt++;
			txn_entry->ready_to_flush = true;
			committed_xids = lappend_int(committed_xids, (int) xid);
		}
		else if (strcmp(operation, "I") == 0 ||
				 strcmp(operation, "U") == 0 ||
				 strcmp(operation, "D") == 0)
		{
			char	   *qualified;
			char	   *undo_sql = NULL;
			StringInfoData colbuf;
			StringInfoData valbuf;
			StringInfoData wherebuf;
			Datum		values[6];
			bool		nulls[6];
			text	   *operation_text;
			text	   *relname_text;
			text	   *owner_text;
			text	   *row_id_text;
			text	   *undo_sql_text = NULL;
			MemoryContext txn_oldcxt;
			ListCell   *pair_lc;

			if (nspname == NULL || relname == NULL || table_owner == NULL ||
				locator_pairs == NIL)
			{
				other_cnt++;
				continue;
			}

			initStringInfo(&wherebuf);
			foreach(pair_lc, locator_pairs)
			{
				HgFlashbackNameVal *pair = lfirst(pair_lc);

				if (pair->name == NULL || pair->val == NULL)
				{
					other_cnt++;
					goto skip_dml;
				}
				if (wherebuf.len > 0)
					appendStringInfoString(&wherebuf, " AND ");
				appendStringInfo(&wherebuf, "%s = %s",
								 quote_identifier(pair->name),
								 pair->val);
			}

			if (wherebuf.len == 0)
			{
				other_cnt++;
				continue;
			}

			if (strcmp(operation, "I") == 0)
			{
				qualified = quote_qualified_identifier(nspname, relname);
				undo_sql = psprintf("DELETE FROM %s WHERE %s",
									qualified,
									wherebuf.data);
				i_cnt++;
			}
			else if (strcmp(operation, "U") == 0)
			{
				if (column_pairs == NIL)
				{
					other_cnt++;
					continue;
				}
				initStringInfo(&colbuf);
				initStringInfo(&valbuf);
				hg_flashback_tx_build_undo_column_lists_from_pairs(&colbuf, &valbuf,
															column_pairs, true);
				qualified = quote_qualified_identifier(nspname, relname);
				undo_sql = psprintf("UPDATE %s SET %s WHERE %s",
									qualified,
									colbuf.data,
									wherebuf.data);
				u_cnt++;
			}
			else
			{
				if (column_pairs == NIL)
				{
					other_cnt++;
					continue;
				}
				initStringInfo(&colbuf);
				initStringInfo(&valbuf);
				hg_flashback_tx_build_undo_column_lists_from_pairs(&colbuf, &valbuf,
															column_pairs, false);
				qualified = quote_qualified_identifier(nspname, relname);
				undo_sql = psprintf("INSERT INTO %s (%s) VALUES (%s)",
									qualified,
									colbuf.data,
									valbuf.data);
				d_cnt++;
			}

			memset(nulls, 0, sizeof(nulls));
			operation_text = cstring_to_text(operation);
			relname_text = cstring_to_text(relname);
			owner_text = cstring_to_text(table_owner);
			row_id_text = cstring_to_text(wherebuf.data);
			if (undo_sql != NULL)
				undo_sql_text = cstring_to_text(undo_sql);

			values[0] = PointerGetDatum(operation_text);
			values[1] = PointerGetDatum(relname_text);
			values[2] = PointerGetDatum(owner_text);
			values[3] = PointerGetDatum(row_id_text);
			if (undo_sql_text != NULL)
				values[4] = PointerGetDatum(undo_sql_text);
			else
			{
				values[4] = (Datum) 0;
				nulls[4] = true;
			}
			values[5] = TimestampTzGetDatum(commit_time);

			txn_oldcxt = MemoryContextSwitchTo(pending_txn_mcxt);
			tuplestore_putvalues(txn_entry->entries, pending_entry_tupdesc, values, nulls);
			txn_entry->entry_count++;
			MemoryContextSwitchTo(txn_oldcxt);

			pfree(operation_text);
			pfree(relname_text);
			pfree(owner_text);
			pfree(row_id_text);
			if (undo_sql_text != NULL)
				pfree(undo_sql_text);
skip_dml:
			;
		}
		else if (strcmp(operation, "X") == 0)
		{
			x_cnt++;
		}
		else
		{
			other_cnt++;
		}
	}

	if (decode_cb_data->write_calls > 0)
		elog(DEBUG1, "hg_flashback_tx decode writes=%llu B=%llu C=%llu I=%llu U=%llu D=%llu X=%llu O=%llu",
			 (unsigned long long) decode_cb_data->write_calls,
			 (unsigned long long) b_cnt,
			 (unsigned long long) c_cnt,
			 (unsigned long long) i_cnt,
			 (unsigned long long) u_cnt,
			 (unsigned long long) d_cnt,
			 (unsigned long long) x_cnt,
			 (unsigned long long) other_cnt);

	MemoryContextSwitchTo(oldcxt);
	return committed_xids;
}

static uint64
hg_flashback_tx_flush_committed_txns(List *committed_xids)
{
	ListCell   *lc;
	uint64		flushed_entries = 0;

	if (committed_xids == NIL)
		return 0;

	if (SPI_connect() != SPI_OK_CONNECT)
		elog(ERROR, "SPI_connect failed");
	PushActiveSnapshot(GetTransactionSnapshot());

	foreach(lc, committed_xids)
	{
		TransactionId xid = (TransactionId) lfirst_int(lc);
		HgFlashbackPendingTxn *txn_entry;

		txn_entry = hash_search(pending_txn_hash, &xid, HASH_FIND, NULL);
		if (txn_entry == NULL || !txn_entry->ready_to_flush)
			continue;

		if (enable_flashback_transaction_query &&
			txn_entry->entries != NULL &&
			txn_entry->entry_count > 0)
		{
			const int	batch_rows = 256;
			Oid			argtypes[1];
			Datum		values[1];
			char		nulls[1];
			uint64		seq = 0;
			StringInfoData sql;
			int			total_count = 0;
			int			stmt_count = 0;
			TupleTableSlot *slot;

			argtypes[0] = INT8OID;
			values[0] = Int64GetDatum((int64) xid);
			nulls[0] = ' ';
			if (SPI_execute_with_args(
					"SELECT COALESCE(MAX(\"undo_change#\"), 0) "
					"FROM hg_flashback_tx.flashback_transaction_query "
					"WHERE xid = $1",
					1, argtypes, values, nulls, true, 1) != SPI_OK_SELECT)
				elog(ERROR, "SPI_execute_with_args failed for undo_change lookup");

			if (SPI_processed == 1)
			{
				char	   *max_seq_str;

				max_seq_str = SPI_getvalue(SPI_tuptable->vals[0],
										   SPI_tuptable->tupdesc, 1);
				if (max_seq_str != NULL)
					seq = (uint64) strtoull(max_seq_str, NULL, 10);
			}

			initStringInfo(&sql);
			appendStringInfoString(&sql,
								   "INSERT INTO hg_flashback_tx.flashback_transaction_query "
								   "(xid, operation, table_name, table_owner, "
								   "row_id, start_timestamp, commit_timestamp, undo_sql, "
								   "\"undo_change#\") VALUES ");

			slot = MakeSingleTupleTableSlot(pending_entry_tupdesc, &TTSOpsMinimalTuple);
			tuplestore_rescan(txn_entry->entries);
			while (tuplestore_gettupleslot(txn_entry->entries, true, false, slot))
			{
				Datum		datum;
				bool		isnull;
				char	   *operation;
				char	   *relname;
				char	   *table_owner;
				char	   *row_id;
				char	   *undo_sql = NULL;
				TimestampTz commit_time = 0;

				if (stmt_count > 0)
					appendStringInfoString(&sql, ", ");

				seq++;
				datum = slot_getattr(slot, 1, &isnull);
				operation = isnull ? pstrdup("") : TextDatumGetCString(datum);
				datum = slot_getattr(slot, 2, &isnull);
				relname = isnull ? pstrdup("") : TextDatumGetCString(datum);
				datum = slot_getattr(slot, 3, &isnull);
				table_owner = isnull ? pstrdup("") : TextDatumGetCString(datum);
				datum = slot_getattr(slot, 4, &isnull);
				row_id = isnull ? pstrdup("") : TextDatumGetCString(datum);
				datum = slot_getattr(slot, 5, &isnull);
				if (!isnull)
					undo_sql = TextDatumGetCString(datum);
				datum = slot_getattr(slot, 6, &isnull);
				if (!isnull)
					commit_time = DatumGetTimestampTz(datum);

				/* Keep row shape identical to legacy path for compatibility. */
				if (commit_time != 0)
					appendStringInfo(&sql, "(%ld, %s, %s, %s, %s, NULL, %s, %s, %ld)",
									 (long) xid,
									 quote_literal_cstr(operation),
									 quote_literal_cstr(relname),
									 quote_literal_cstr(table_owner),
									 quote_literal_cstr(row_id),
									 quote_literal_cstr(timestamptz_to_str(commit_time)),
									 undo_sql ? quote_literal_cstr(undo_sql) : "NULL",
									 (long) seq);
				else
					appendStringInfo(&sql, "(%ld, %s, %s, %s, %s, NULL, NULL, %s, %ld)",
									 (long) xid,
									 quote_literal_cstr(operation),
									 quote_literal_cstr(relname),
									 quote_literal_cstr(table_owner),
									 quote_literal_cstr(row_id),
									 undo_sql ? quote_literal_cstr(undo_sql) : "NULL",
									 (long) seq);

				pfree(operation);
				pfree(relname);
				pfree(table_owner);
				pfree(row_id);
				if (undo_sql != NULL)
					pfree(undo_sql);
				ExecClearTuple(slot);
				stmt_count++;
				total_count++;

				if (stmt_count >= batch_rows)
				{
					if (SPI_execute(sql.data, false, 0) != SPI_OK_INSERT)
						elog(ERROR, "SPI_execute failed for persistent batch insert");
					resetStringInfo(&sql);
					appendStringInfoString(&sql,
										   "INSERT INTO hg_flashback_tx.flashback_transaction_query "
										   "(xid, operation, table_name, table_owner, "
										   "row_id, start_timestamp, commit_timestamp, undo_sql, "
										   "\"undo_change#\") VALUES ");
					stmt_count = 0;
				}
			}
			ExecDropSingleTupleTableSlot(slot);

			if (stmt_count > 0 &&
				SPI_execute(sql.data, false, 0) != SPI_OK_INSERT)
				elog(ERROR, "SPI_execute failed for persistent batch insert");
			flushed_entries += total_count;
			pfree(sql.data);
		}

		/* xid is fully persisted (or recording disabled), safe to drop pending. */
		hg_flashback_tx_free_entries(txn_entry->entries);
		hash_search(pending_txn_hash, &xid, HASH_REMOVE, NULL);
	}

	PopActiveSnapshot();
	SPI_finish();

	return flushed_entries;
}

static void
hg_flashback_tx_free_entries(Tuplestorestate *entries)
{
	if (entries != NULL)
		tuplestore_end(entries);
}

/* Ensure the logical replication slot exists. */
static void
hg_flashback_tx_ensure_slot(void)
{
	char	   *slot_name;

	StartTransactionCommand();
	if (SPI_connect() != SPI_OK_CONNECT)
		elog(ERROR, "SPI_connect failed");

	PushActiveSnapshot(GetTransactionSnapshot());

	slot_name = psprintf("hg_flashback_tx_slot_%s", current_db_name);

	if (SPI_execute_with_args(
			"SELECT 1 FROM pg_replication_slots WHERE slot_name = $1",
			1, (Oid[]) { TEXTOID }, (Datum[]) { CStringGetTextDatum(slot_name) },
			NULL, true, 1) != SPI_OK_SELECT)
		elog(ERROR, "SPI_execute failed for slot check");

	if (SPI_processed == 0)
	{
		if (SPI_execute_with_args(
				"SELECT * FROM pg_create_logical_replication_slot($1, 'flashback_decoder')",
				1, (Oid[]) { TEXTOID }, (Datum[]) { CStringGetTextDatum(slot_name) },
				NULL, false, 0) != SPI_OK_SELECT)
			elog(ERROR, "SPI_execute failed for slot creation");
	}

	pfree(slot_name);

	PopActiveSnapshot();
	SPI_finish();
	CommitTransactionCommand();
}

/* Purge old rows from flashback_transaction_query. */
static void
hg_flashback_tx_purge_old_rows(void)
{
	TimestampTz	cutoff_time;
	Oid			argtypes[1];
	Datum			values[1];
	char			nulls[1];

	/* If undo_retention is 0, no purge needed */
	if (undo_retention <= 0)
		return;
	
	/* Delete rows whose commit timestamp is older than retention window. */
	cutoff_time = GetCurrentTimestamp() - (undo_retention * 1000000LL);

	StartTransactionCommand();
	if (SPI_connect() != SPI_OK_CONNECT)
		elog(ERROR, "SPI_connect failed");

	PushActiveSnapshot(GetTransactionSnapshot());

	argtypes[0] = TIMESTAMPTZOID;
	values[0] = TimestampTzGetDatum(cutoff_time);
	nulls[0] = ' ';

	if (SPI_execute_with_args(
			"DELETE FROM hg_flashback_tx.flashback_transaction_query "
			"WHERE commit_timestamp IS NOT NULL "
			"AND commit_timestamp < $1",
			1, argtypes, values, nulls, false, 0) != SPI_OK_DELETE)
		elog(ERROR, "SPI_execute_with_args failed for purge");

	PopActiveSnapshot();
	SPI_finish();
	CommitTransactionCommand();
}

/*
 * Build column and value lists for UPDATE/INSERT undo SQL.
 *
 * When for_update=true: outputs "col1=val1, col2=val2, ..." format for UPDATE SET clause.
 * When for_update=false: outputs separate column list and value list for INSERT.
 */
static void
hg_flashback_tx_build_undo_column_lists_from_pairs(StringInfo colbuf, StringInfo valbuf,
												 List *pairs, bool for_update)
{
	ListCell   *lc;

	foreach(lc, pairs)
	{
		HgFlashbackNameVal *pair = lfirst(lc);
		char	   *colname = pair->name;
		char	   *val = pair->val;

		if (colname == NULL || val == NULL)
			elog(ERROR, "invalid change format");

		if (for_update)
		{
			if (colbuf->len > 0)
				appendStringInfoString(colbuf, ", ");
			appendStringInfo(colbuf, "%s=%s",
							 quote_identifier(colname),
							 val);
		}
		else
		{
			if (colbuf->len > 0)
				appendStringInfoString(colbuf, ", ");
			appendStringInfoString(colbuf, quote_identifier(colname));
		}

		if (valbuf->len > 0)
			appendStringInfoString(valbuf, ", ");
		appendStringInfoString(valbuf, val);
	}
}

/*
 * Check if the table has REPLICA IDENTITY FULL set (using SPI).
 * This version is for use after CREATE TABLE is committed.
 */
static bool
hg_flashback_tx_has_replica_identity_full(Oid relid)
{
	bool		result = false;
	int			ret;
	char		query[256];

	snprintf(query, sizeof(query),
			 "SELECT relreplident = 'f' FROM pg_class WHERE oid = %u", relid);

	ret = SPI_execute(query, true, 0);
	if (ret == SPI_OK_SELECT && SPI_processed > 0)
	{
		bool		is_full;
		Datum		datum;
		bool		isnull;

		datum = SPI_getbinval(SPI_tuptable->vals[0],
							 SPI_tuptable->tupdesc, 1, &isnull);
		is_full = DatumGetBool(datum);
		result = is_full;
	}

	return result;
}

/*
 * ProcessUtility_hook: Set REPLICA IDENTITY FULL for tables with primary key
 * after CREATE TABLE is executed.
 */
static void
hg_flashback_tx_utility_hook(PlannedStmt *pstmt,
						  const char *queryString,
						  bool readOnlyTree,
						  ProcessUtilityContext context,
						  ParamListInfo params,
						  QueryEnvironment *queryEnv,
						  DestReceiver *dest,
						  QueryCompletion *qc)
{
	Node	   *stmt;

	if (prev_ProcessUtility_hook)
		prev_ProcessUtility_hook(pstmt, queryString, readOnlyTree,
								  context, params, queryEnv, dest, qc);
	else
		standard_ProcessUtility(pstmt, queryString, readOnlyTree,
								 context, params, queryEnv, dest, qc);

	/*
	 * Check after the utility execution to see if we need to set
	 * REPLICA IDENTITY FULL for the newly created table.
	 */
	if (!enable_auto_replica_identity)
		return;

	stmt = pstmt->utilityStmt;

	if (!IsA(stmt, CreateStmt))
		return;

	{
		CreateStmt *create = (CreateStmt *) stmt;
		Oid			tableOid;
		Oid			nspOid;
		char	   *nspname;
		char	   *relname;
		char		query[512];
		int			ret;

		if (create->relation == NULL)
			return;

		relname = create->relation->relname;
		if (relname == NULL)
			return;

		if (create->relation->schemaname != NULL)
			nspOid = get_namespace_oid(create->relation->schemaname, false);
		else
			nspOid = RangeVarGetCreationNamespace(create->relation);
		nspname = get_namespace_name(nspOid);
		if (nspname == NULL)
			return;

		if (strcmp(nspname, "pg_catalog") == 0 ||
			strcmp(nspname, "information_schema") == 0 ||
			strcmp(nspname, "hg_flashback_tx") == 0 ||
			strcmp(nspname, "hg_security") == 0)
			return;

		tableOid = get_relname_relid(relname, nspOid);
		if (!OidIsValid(tableOid))
			return;

		if (SPI_connect() != SPI_OK_CONNECT)
			return;

		if (hg_flashback_tx_has_replica_identity_full(tableOid))
		{
			SPI_finish();
			return;
		}

		elog(LOG, "hg_flashback_tx: setting REPLICA IDENTITY FULL for table %s.%s",
			 nspname, relname);

		snprintf(query, sizeof(query),
				 "ALTER TABLE %s.%s REPLICA IDENTITY FULL",
				 quote_identifier(nspname),
				 quote_identifier(relname));

		ret = SPI_execute(query, false, 0);
		if (ret != SPI_OK_UTILITY)
			elog(LOG, "hg_flashback_tx: failed to set REPLICA IDENTITY FULL for %s.%s",
				 nspname, relname);

		SPI_finish();
	}
}
