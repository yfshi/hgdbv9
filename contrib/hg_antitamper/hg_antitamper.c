/*-------------------------------------------------------------------------
 *
 * hg_antitamper.c
 *	  
 *
 * Authored by xuji@highgo.com, 20251223.
 *
 * Copyright:
 * Copyright (c) 2009-2025, HighGo Software Co.,Ltd. All right reserved.
 *
 * IDENTIFICATION
 *	  contrib/hg_antitamper/hg_antitamper.c
 *
 *-------------------------------------------------------------------------
 */

/* Begin - ReqID: SRS-SECURITY-ANTITAMPERING */

#include "postgres.h"

#include "access/htup.h"
#include "access/tupdesc.h"
#include "access/heapam.h"
#include "access/table.h"
#include "access/visibilitymap.h"
#include "catalog/objectaccess.h"
#include "catalog/objectaddress.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_seclabel.h"
#include "catalog/pg_shseclabel.h"
#include "catalog/namespace.h"
#include "catalog/pg_collation.h"
#include "catalog/pg_namespace.h"
#include "commands/seclabel.h"
#include "commands/schemacmds.h"
#include "executor/executor.h"
#include "executor/tuptable.h"
#include "executor/spi.h"
#include "funcapi.h"
#include "parser/parse_func.h"
#include "miscadmin.h"
#include "storage/ipc.h"
#include "storage/shmem.h"
#include "tcop/utility.h"
#include "utils/acl.h"
#include "utils/builtins.h"
#include "utils/catcache.h"
#include "utils/date.h"
#include "utils/fmgroids.h"
#include "utils/guc.h"
#include "utils/rls.h"
#include "utils/syscache.h"
#include "utils/lsyscache.h"
#include "utils/relcache.h"
#include "utils/uuid.h"
#include "utils/rel.h"
#include "nodes/bitmapset.h"
#include "nodes/parsenodes.h"
#include "nodes/execnodes.h"
#include "se_features_interface.h"
#include "catalog/pg_authid_d.h"

PG_MODULE_MAGIC;

void		_PG_init(void);
void		_PG_fini(void);

bool hg_antitamper_enabled = false;

#define HG_SYS_SCHEMA_ANTITAMPER "schema_antitamper"
#define Natts_schema_antitamper (2)
#define Anum_schema_antitamper_schemaoid_col (1)
#define Anum_schema_antitamper_antitamper_col (2)

#define HG_HASH_COLUMN_NAME "__hg_hash__"
#define HG_UINT32_MASK       UINT64CONST(4294967295)
#define HG_SECURITY_SCHEMA					"hg_security"
#define HG_GLOBAL_ANTITAMPER_TABLE			"hg_global_antitamper"
/*
 * Common execution state.
 *
 * user_switched and spi_connected are changed after PG_TRY establishes
 * its jump point and are inspected in PG_CATCH, so they are volatile.
 */
typedef struct HgHistoryExecContext
{
	Relation relation;
	Oid table_owner;
	Oid saved_userid;
	int saved_sec_context;
	char *qualified_name;

	volatile bool user_switched;
	volatile bool spi_connected;
} HgHistoryExecContext;

PG_FUNCTION_INFO_V1(get_history_hash_diff_internal);
PG_FUNCTION_INFO_V1(append_history_hash_diff_internal);

/*
 * Open and validate a history table before entering PG_TRY.
 * get table info.
 * This function does not switch user identity and does not connect SPI.
 * Once it returns successfully, context->relation remains open with
 * AccessShareLock until the caller closes it.
 */
static void
hg_prepare_history_operation(Oid hist_table_oid, bool require_rec_num, HgHistoryExecContext *context)
{
	Relation relation;
	char relkind;
	char *namespace_name;
	const char *relation_name;

	MemSet(context, 0, sizeof(*context));

	relation = table_open(hist_table_oid, AccessShareLock);
	relkind = RelationGetForm(relation)->relkind;

	if (relkind != RELKIND_RELATION &&
		relkind != RELKIND_PARTITIONED_TABLE)
	{
		table_close(relation, AccessShareLock);
		ereport(ERROR,
				(errcode(ERRCODE_WRONG_OBJECT_TYPE),
				 errmsg("object with OID %u is not a supported history table",
						hist_table_oid),
				 errdetail("Relation kind is '%c'.", relkind)));
	}

	namespace_name = get_namespace_name(RelationGetNamespace(relation));

	relation_name = RelationGetRelationName(relation);
	if (namespace_name == NULL)
	{
		table_close(relation, AccessShareLock);
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_SCHEMA),
				 errmsg("namespace of relation with OID %u does not exist",
						hist_table_oid)));
	}

	/*
	 * Restrict this privileged API to extension-managed history tables.
	 */
	if (strcmp(namespace_name, HG_SECURITY_SCHEMA) != 0)
	{
		char *actual_name;

		actual_name = quote_qualified_identifier(namespace_name, relation_name);
		table_close(relation, AccessShareLock);
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("relation %s is not a valid history table",
						actual_name),
				 errdetail("History tables must be named "
						   "hg_security.hist_<table_oid>.")));
	}

	if (get_attnum(hist_table_oid, "hash_ins") == InvalidAttrNumber ||
		get_attnum(hist_table_oid, "hash_del") == InvalidAttrNumber ||
		(require_rec_num && get_attnum(hist_table_oid, "rec_num") == InvalidAttrNumber))
	{
		table_close(relation, AccessShareLock);
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_COLUMN),
				 errmsg("history table %s.%s has an invalid structure",
						namespace_name, relation_name),
				 errdetail(require_rec_num
							   ? "Columns rec_num, hash_ins and hash_del must exist."
							   : "Columns hash_ins and hash_del must exist.")));
	}

	context->table_owner = RelationGetForm(relation)->relowner;

	if (!OidIsValid(context->table_owner))
	{
		table_close(relation, AccessShareLock);
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("owner of history table %s.%s is invalid",
						namespace_name, relation_name)));
	}

	context->relation = relation;
	context->qualified_name = quote_qualified_identifier(namespace_name, relation_name);

	context->user_switched = false;
	context->spi_connected = false;

	/*
	 * Save the identity that entered the SQL-callable C function.
	 *
	 * When the caller is a SECURITY DEFINER PL/pgSQL function, this is the
	 * outer function owner, not session_user.
	 */
	GetUserIdAndSecContext(&context->saved_userid, &context->saved_sec_context);
}


/*
 * Common start sequence used by both SQL-callable functions.
 * switch user permissions to table owner user.
 * Must be called inside the caller's PG_TRY block.
 */
static void
hg_start_history_operation(HgHistoryExecContext *context)
{
	int spi_rc;

	SetUserIdAndSecContext(context->table_owner, context->saved_sec_context | SECURITY_LOCAL_USERID_CHANGE);

	context->user_switched = true;
	spi_rc = SPI_connect();
	if (spi_rc != SPI_OK_CONNECT)
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("SPI_connect failed"),
				 errdetail("SPI_connect returned %d.", spi_rc)));

	context->spi_connected = true;
}


/*
 * Common normal-path finish sequence.
 * Restore the entering identity before SPI_finish() so the privileged
 * table-owner identity is held for the shortest possible time.
 * switch user permissions to old user.
 * Must be called inside the caller's PG_TRY block.
 */
static void
hg_finish_history_operation(HgHistoryExecContext *context)
{
	int spi_rc;

	if (context->user_switched)
	{
		SetUserIdAndSecContext(context->saved_userid, context->saved_sec_context);
		context->user_switched = false;
	}

	if (context->spi_connected)
	{
		spi_rc = SPI_finish();

		/*
		 * SPI_finish() has already returned. Clear the flag before possibly
		 * raising another ERROR, so PG_CATCH will not call SPI_finish twice.
		 */
		context->spi_connected = false;
		if (spi_rc != SPI_OK_FINISH)
			ereport(ERROR,
					(errcode(ERRCODE_INTERNAL_ERROR),
					 errmsg("SPI_finish failed"),
					 errdetail("SPI_finish returned %d.", spi_rc)));
	}
}

/*
 * Common PG_CATCH cleanup.
 * switch user permissions to old user.
 * This function intentionally does not ereport(ERROR); it performs best-effort
 * restoration and lets the caller preserve the original error with
 * PG_RE_THROW().
 */
static void
hg_abort_history_operation(HgHistoryExecContext *context)
{
	if (context->user_switched)
	{
		SetUserIdAndSecContext(context->saved_userid, context->saved_sec_context);
		context->user_switched = false;
	}
}

/*
 * Extract one bigint value from the first column of exactly one SPI row.
 * get value: sum(hash_ins) -sum(hash_del)
 */
static int64
hg_spi_get_single_int64(const char *result_name, bool null_as_zero)
{
	HeapTuple tuple;
	TupleDesc tuple_desc;
	Datum result_datum;
	bool result_is_null;

	if (SPI_tuptable == NULL || SPI_processed != 1)
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("unexpected result from %s", result_name),
				 errdetail("Expected one row, got %llu.", (unsigned long long)SPI_processed)));

	tuple = SPI_tuptable->vals[0];
	tuple_desc = SPI_tuptable->tupdesc;

	result_datum = SPI_getbinval(tuple, tuple_desc, 1, &result_is_null);

	if (result_is_null)
	{
		if (null_as_zero)
			return 0;

		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				 errmsg("%s must not be null", result_name)));
	}

	return DatumGetInt64(result_datum);
}


/* -------------------------------------------------------------------------
 * Read history-table hash difference
 * -------------------------------------------------------------------------
 */
/*
 * SQL:
 *     hg_security.get_history_hash_diff_internal(hist_table_oid oid)
 * Returns:
 *     COALESCE(SUM(hash_ins)::bigint, 0) - COALESCE(SUM(hash_del)::bigint, 0)
 */
Datum
get_history_hash_diff_internal(PG_FUNCTION_ARGS)
{
	Oid hist_table_oid;
	HgHistoryExecContext context;
	char *query;
	int64 diff_result = 0;

	if (PG_ARGISNULL(0))
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				 errmsg("history table OID must not be null")));

	hist_table_oid = PG_GETARG_OID(0);

	/*
	 * Read operation needs hash_ins/hash_del, but does not require rec_num.
	 */
	hg_prepare_history_operation(hist_table_oid, false, &context);

	PG_TRY();
	{
		int spi_rc;

		hg_start_history_operation(&context);
		/*
		 * Function-specific read SQL stays directly inside this function.
		 * Steps 2 and 3 are combined into one history-table scan.
		 */
		query = psprintf(
			"SELECT "
			"    COALESCE("
			"        pg_catalog.sum(hash_ins)::pg_catalog.int8, "
			"        0::pg_catalog.int8"
			"    ) "
			"    OPERATOR(pg_catalog.-) "
			"    COALESCE("
			"        pg_catalog.sum(hash_del)::pg_catalog.int8, "
			"        0::pg_catalog.int8"
			"    ) "
			"FROM %s",
			context.qualified_name);

		spi_rc = SPI_execute(query, true, 1);
		if (spi_rc != SPI_OK_SELECT)
			ereport(ERROR,
					(errcode(ERRCODE_INTERNAL_ERROR),
					 errmsg("failed to calculate history hash difference"),
					 errdetail("SPI_execute returned %d.", spi_rc)));

		diff_result = hg_spi_get_single_int64("history table hash difference", true);
		hg_finish_history_operation(&context);
	}
	PG_CATCH();
	{
		hg_abort_history_operation(&context);
		PG_RE_THROW();
	}
	PG_END_TRY();

	table_close(context.relation, AccessShareLock);
	PG_RETURN_INT64(diff_result);
}

/* -------------------------------------------------------------------------
 * Append a correction record to the history table
 * -------------------------------------------------------------------------
 */
/*
 * SQL:
 *     hg_security.append_history_hash_diff_internal(
 *         hist_table_oid oid, diff_value bigint)
 * diff_value > 0:  insert into hash_ins
 * diff_value < 0:  insert ABS(diff_value) into hash_del
 * Return: newly inserted rec_num
 */
Datum
append_history_hash_diff_internal(PG_FUNCTION_ARGS)
{
	Oid hist_table_oid;
	int64 diff_value;
	int64 stored_hash_value;
	bool write_hash_ins;
	int64 inserted_rec_num = 0;
	HgHistoryExecContext context;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				 errmsg("history table OID and difference value must not be null")));

	hist_table_oid = PG_GETARG_OID(0);
	diff_value = PG_GETARG_INT64(1);

	if (diff_value == 0)
		PG_RETURN_INT64(0);

	if (diff_value == PG_INT64_MIN)
		ereport(ERROR,
				(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
				 errmsg("cannot store the absolute value of PG_INT64_MIN")));

	stored_hash_value = diff_value > 0 ? diff_value : -diff_value;
	write_hash_ins = (diff_value > 0);

	/*
	 * Write operation additionally requires rec_num.
	 */
	hg_prepare_history_operation(hist_table_oid, true, &context);

	PG_TRY();
	{
		char *lock_query;
		char *insert_query;
		Oid argtypes[1] = {INT8OID};
		Datum values[1];
		int spi_rc;

		hg_start_history_operation(&context);

		/*
		 * Function-specific write logic stays directly inside this function.
		 * therefore the history table is serialized before calculating and inserting.
		 */
		lock_query = psprintf("LOCK TABLE %s IN SHARE ROW EXCLUSIVE MODE",
							  context.qualified_name);

		spi_rc = SPI_execute(lock_query, false, 0);
		if (spi_rc != SPI_OK_UTILITY)
			ereport(ERROR,
					(errcode(ERRCODE_INTERNAL_ERROR),
					 errmsg("failed to lock history table"),
					 errdetail("SPI_execute returned %d.", spi_rc)));

		if (write_hash_ins)
		{
			insert_query = psprintf(
				"INSERT INTO %s (rec_num, hash_ins, hash_del) "
				"SELECT "
				"    COALESCE("
				"        pg_catalog.max(rec_num)::pg_catalog.int8, "
				"        0::pg_catalog.int8"
				"    ) + 1::pg_catalog.int8, "
				"    $1::pg_catalog.int8, "
				"    NULL::pg_catalog.int8 "
				"FROM %s "
				"RETURNING rec_num::pg_catalog.int8",
				context.qualified_name,
				context.qualified_name);
		}
		else
		{
			insert_query = psprintf(
				"INSERT INTO %s (rec_num, hash_ins, hash_del) "
				"SELECT "
				"    COALESCE("
				"        pg_catalog.max(rec_num)::pg_catalog.int8, "
				"        0::pg_catalog.int8"
				"    ) + 1::pg_catalog.int8, "
				"    NULL::pg_catalog.int8, "
				"    $1::pg_catalog.int8 "
				"FROM %s "
				"RETURNING rec_num::pg_catalog.int8",
				context.qualified_name,
				context.qualified_name);
		}
		values[0] = Int64GetDatum(stored_hash_value);

		spi_rc = SPI_execute_with_args(insert_query, 1, argtypes, values, NULL, false, 1);
		if (spi_rc != SPI_OK_INSERT_RETURNING)
			ereport(ERROR,
					(errcode(ERRCODE_INTERNAL_ERROR),
					 errmsg("failed to append history hash difference"),
					 errdetail("SPI_execute_with_args returned %d.", spi_rc)));

		inserted_rec_num = hg_spi_get_single_int64("inserted history rec_num", false);
		hg_finish_history_operation(&context);
	}
	PG_CATCH();
	{
		hg_abort_history_operation(&context);
		PG_RE_THROW();
	}
	PG_END_TRY();

	table_close(context.relation, AccessShareLock);
	PG_RETURN_INT64(inserted_rec_num);
}

PG_FUNCTION_INFO_V1(insert_global_antitamper_internal);

/*
 * Select the temporary execution identity used for inserting into
 * hg_security.hg_global_antitamper.
 *
 * Separation of powers enabled:
 *     use the bootstrap superuser.
 *
 * Separation of powers disabled:
 *     use role "sso".
 */
static Oid
hg_get_global_antitamper_exec_user(void)
{
	if (se_feature_mode)
		return get_role_oid(SYSSSO_ROLE, false);
	else
		return BOOTSTRAP_SUPERUSERID;
}


/*
 * Prepare the fixed global antitamper table for a privileged INSERT.
 *
 * This function reuses HgHistoryExecContext so that the existing
 * hg_start_history_operation(), hg_finish_history_operation(), and
 * hg_abort_history_operation() functions can be called unchanged.
 */
static void
hg_prepare_global_antitamper_operation(HgHistoryExecContext *context)
{
	Oid namespace_oid;
	Oid relation_oid;
	Relation relation;
	char *namespace_name;
	const char *relation_name;

	MemSet(context, 0, sizeof(*context));

	namespace_oid = get_namespace_oid(HG_SECURITY_SCHEMA, false);
	relation_oid = get_relname_relid(HG_GLOBAL_ANTITAMPER_TABLE, namespace_oid);

	if (!OidIsValid(relation_oid))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_TABLE),
				 errmsg("relation \"%s.%s\" does not exist",
						HG_SECURITY_SCHEMA, HG_GLOBAL_ANTITAMPER_TABLE)));

	relation = table_open(relation_oid, RowExclusiveLock);

	namespace_name = get_namespace_name(RelationGetNamespace(relation));
	relation_name = RelationGetRelationName(relation);

	if (namespace_name == NULL ||
		strcmp(namespace_name, HG_SECURITY_SCHEMA) != 0 ||
		strcmp(relation_name, HG_GLOBAL_ANTITAMPER_TABLE) != 0)
	{
		table_close(relation, RowExclusiveLock);

		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("unexpected global antitamper table")));
	}

	if (get_attnum(relation_oid, "username") == InvalidAttrNumber ||
		get_attnum(relation_oid, "dbname") == InvalidAttrNumber ||
		get_attnum(relation_oid, "schemaoid") == InvalidAttrNumber ||
		get_attnum(relation_oid, "reloid") == InvalidAttrNumber ||
		get_attnum(relation_oid, "insertrelhash") == InvalidAttrNumber ||
		get_attnum(relation_oid, "deleterelhash") == InvalidAttrNumber ||
		get_attnum(relation_oid, "starttime") == InvalidAttrNumber ||
		get_attnum(relation_oid, "sqlcommand") == InvalidAttrNumber)
	{
		table_close(relation, RowExclusiveLock);

		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_COLUMN),
				 errmsg("table \"%s.%s\" has an invalid structure",
						HG_SECURITY_SCHEMA,
						HG_GLOBAL_ANTITAMPER_TABLE)));
	}

	context->relation = relation;
	context->table_owner = hg_get_global_antitamper_exec_user();
	context->qualified_name = quote_qualified_identifier(namespace_name, relation_name);
	context->user_switched = false;
	context->spi_connected = false;

	GetUserIdAndSecContext(&context->saved_userid, &context->saved_sec_context);
}

/*
 * Insert one global antitamper record.
 *
 * SQL signature:
 *
 *     insert_global_antitamper_internal(
 *         username       text,
 *         dbname         text,
 *         schema_oid     oid,
 *         relation_oid   oid,
 *         insert_hash    bigint,
 *         delete_hash    bigint,
 *         sql_command    text,
 *         caller         text
 *     ) returns void
 *
 * insert_hash, delete_hash and sql_command may be NULL, so the SQL
 * function must not be declared STRICT.
 */
Datum
insert_global_antitamper_internal(PG_FUNCTION_ARGS)
{
	HgHistoryExecContext context;
	Oid schema_oid;
	Oid relation_oid;
	char *caller_name = NULL;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2) || PG_ARGISNULL(3))
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				 errmsg("username, database name, schema OID and relation OID must not be null")));
	}

	schema_oid = PG_GETARG_OID(2);
	relation_oid = PG_GETARG_OID(3);

	if (!PG_ARGISNULL(7))
	{
		caller_name = text_to_cstring(PG_GETARG_TEXT_PP(7));
	}

	if (caller_name == NULL || strcmp(caller_name, "__set_generic_hash__") != 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("permission denied")));
	}

	hg_prepare_global_antitamper_operation(&context);

	PG_TRY();
	{
		char *query;
		Oid argtypes[7] = {TEXTOID, TEXTOID, OIDOID, OIDOID, INT8OID, INT8OID, TEXTOID};
		Datum values[7];
		char nulls[7] = {' ', ' ', ' ', ' ', ' ', ' ', ' '};
		int spi_rc;

		hg_start_history_operation(&context);

		query = psprintf(
			"INSERT INTO %s "
			"("
			"    username, "
			"    dbname, "
			"    schemaoid, "
			"    reloid, "
			"    starttime, "
			"    insertrelhash, "
			"    deleterelhash, "
			"    sqlcommand"
			") "
			"VALUES "
			"("
			"    $1::pg_catalog.text, "
			"    $2::pg_catalog.text, "
			"    $3::pg_catalog.oid, "
			"    $4::pg_catalog.oid, "
			"    CURRENT_TIMESTAMP::timestamp without time zone, "
			"    $5::pg_catalog.int8, "
			"    $6::pg_catalog.int8, "
			"    $7::pg_catalog.text"
			")",
			context.qualified_name);

		values[0] = PG_GETARG_DATUM(0);
		values[1] = PG_GETARG_DATUM(1);
		values[2] = ObjectIdGetDatum(schema_oid);
		values[3] = ObjectIdGetDatum(relation_oid);

		if (PG_ARGISNULL(4))
		{
			values[4] = (Datum)0;
			nulls[4] = 'n';
		}
		else
			values[4] = PG_GETARG_DATUM(4);

		if (PG_ARGISNULL(5))
		{
			values[5] = (Datum)0;
			nulls[5] = 'n';
		}
		else
			values[5] = PG_GETARG_DATUM(5);

		if (PG_ARGISNULL(6))
		{
			values[6] = (Datum)0;
			nulls[6] = 'n';
		}
		else
			values[6] = PG_GETARG_DATUM(6);

		spi_rc = SPI_execute_with_args(query, 7, argtypes, values, nulls, false, 1);

		if (spi_rc != SPI_OK_INSERT)
			ereport(ERROR,
					(errcode(ERRCODE_INTERNAL_ERROR),
					 errmsg("failed to insert global antitamper record"),
					 errdetail("SPI_execute_with_args returned %d.", spi_rc)));

		if (SPI_processed != 1)
			ereport(ERROR,
					(errcode(ERRCODE_INTERNAL_ERROR),
					 errmsg("unexpected number of inserted rows"),
					 errdetail("Expected one row, inserted %llu.", (unsigned long long)SPI_processed)));

		hg_finish_history_operation(&context);
	}
	PG_CATCH();
	{
		/*
		 * Reuse the existing error-path helper to restore the user identity,
		 * then immediately propagate the original ERROR.
		 */
		hg_abort_history_operation(&context);
		PG_RE_THROW();
	}
	PG_END_TRY();

	table_close(context.relation, RowExclusiveLock);
	PG_RETURN_VOID();
}

PG_FUNCTION_INFO_V1(get_table_hash_sum);
/*
 * get_table_hash_sum:
 *     get table hash total value
 */
Datum
get_table_hash_sum(PG_FUNCTION_ARGS)
{
	text *table_name_text;
	text *schema_name_text;
	char *table_name;
	char *schema_name;
	char *actual_schema_name;
	char *qualified_relation_name;
	Oid namespace_oid;
	Oid table_oid;
	Oid table_owner_oid;
	Oid saved_userid;
	int saved_sec_context;
	Relation relation;
	TupleDesc tuple_desc;
	StringInfoData row_expression;
	StringInfoData query;
	volatile bool user_switched = false;
	volatile int64 sum_result = 0;
	char relkind;
	int i;

	table_name_text = PG_GETARG_TEXT_PP(0);
	schema_name_text = PG_GETARG_TEXT_PP(1);

	table_name = text_to_cstring(table_name_text);
	schema_name = text_to_cstring(schema_name_text);

	/* get table oid */
	namespace_oid = LookupExplicitNamespace(schema_name, false);
	table_oid = get_relname_relid(table_name, namespace_oid);

	if (!OidIsValid(table_oid))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_TABLE),
				 errmsg("relation %s.%s does not exist",
						quote_identifier(schema_name),
						quote_identifier(table_name))));

	relation = table_open(table_oid, AccessShareLock);
	relkind = RelationGetForm(relation)->relkind;

	/* RELOPT_KIND_HEAP | RELOPT_KIND_TOAST | RELOPT_KIND_PARTITIONED */
	if (relkind != RELKIND_RELATION &&
		relkind != RELKIND_PARTITIONED_TABLE &&
		relkind != RELKIND_TOASTVALUE)
	{
		table_close(relation, AccessShareLock);
		ereport(ERROR,
				(errcode(ERRCODE_WRONG_OBJECT_TYPE),
				 errmsg("object with OID %u is not a supported table", table_oid),
				 errdetail("Relation kind is '%c'.", relkind)));
	}

	table_owner_oid = RelationGetForm(relation)->relowner;
	tuple_desc = RelationGetDescr(relation);

	initStringInfo(&row_expression);

	/* get TupleDesc attnum and exclude  __hg_hash__ */
	for (i = 0; i < tuple_desc->natts; i++)
	{
		Form_pg_attribute attr;
		const char *column_name;
		const char *quoted_column_name;

		attr = TupleDescAttr(tuple_desc, i);

		if (attr->attnum <= 0 || attr->attisdropped)
			continue;

		column_name = NameStr(attr->attname);
		if (strcmp(column_name, HG_HASH_COLUMN_NAME) == 0)
			continue;

		quoted_column_name = quote_identifier(column_name);

		if (row_expression.len > 0)
			appendStringInfoString(&row_expression,	" OPERATOR(pg_catalog.||) ");

		appendStringInfo(
			&row_expression,
			"((pg_catalog.hashtext("
			"COALESCE(%s::pg_catalog.text, ''::pg_catalog.text)"
			")::pg_catalog.int8 "
			"OPERATOR(pg_catalog.&) " UINT64_FORMAT
			"::pg_catalog.int8)::pg_catalog.text)",
			quoted_column_name,
			HG_UINT32_MASK);
	}

	if (row_expression.len == 0)
	{
		table_close(relation, AccessShareLock);
		PG_RETURN_INT64(0);
	}

	actual_schema_name = get_namespace_name(RelationGetNamespace(relation));
	if (actual_schema_name == NULL)
	{
		table_close(relation, AccessShareLock);
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_SCHEMA),
				 errmsg("could not resolve namespace for relation OID %u", table_oid)));
	}

	qualified_relation_name =
		quote_qualified_identifier(actual_schema_name, RelationGetRelationName(relation));

	initStringInfo(&query);
	appendStringInfo(
		&query,
		"SELECT COALESCE("
		"pg_catalog.sum(("
		"pg_catalog.hashtext(%s)::pg_catalog.int8 "
		"OPERATOR(pg_catalog.&) " UINT64_FORMAT
		"::pg_catalog.int8)), "
		"0::pg_catalog.numeric"
		")::pg_catalog.int8 "
		"FROM %s",
		row_expression.data,
		HG_UINT32_MASK,
		qualified_relation_name);

	/* save old user context */
	GetUserIdAndSecContext(&saved_userid, &saved_sec_context);

	PG_TRY();
	{
		int spi_rc;
		HeapTuple result_tuple;
		TupleDesc result_desc;
		Datum result_datum;
		bool is_null;

		/* exec SELECT */
		SetUserIdAndSecContext(table_owner_oid,	saved_sec_context | SECURITY_LOCAL_USERID_CHANGE);
		user_switched = true;

		spi_rc = SPI_connect();
		if (spi_rc != SPI_OK_CONNECT)
			ereport(ERROR,
					(errcode(ERRCODE_INTERNAL_ERROR),
					 errmsg("SPI_connect failed"),
					 errdetail("SPI_connect returned %d.", spi_rc)));

		spi_rc = SPI_execute(query.data, true, 0);
		if (spi_rc != SPI_OK_SELECT)
			ereport(ERROR,
					(errcode(ERRCODE_INTERNAL_ERROR),
					 errmsg("failed to calculate hash sum for relation %s",
							qualified_relation_name),
					 errdetail("SPI_execute returned %d.", spi_rc)));

		if (SPI_processed != 1 || SPI_tuptable == NULL)
			ereport(ERROR,
					(errcode(ERRCODE_INTERNAL_ERROR),
					 errmsg("unexpected result while calculating table hash sum"),
					 errdetail("Expected one result row, got " UINT64_FORMAT ".",
							   SPI_processed)));

		result_tuple = SPI_tuptable->vals[0];
		result_desc = SPI_tuptable->tupdesc;
		result_datum = SPI_getbinval(result_tuple, result_desc, 1, &is_null);

		sum_result = is_null ? 0 : DatumGetInt64(result_datum);

		spi_rc = SPI_finish();
		if (spi_rc != SPI_OK_FINISH)
			ereport(ERROR,
					(errcode(ERRCODE_INTERNAL_ERROR),
					 errmsg("SPI_finish failed"),
					 errdetail("SPI_finish returned %d.", spi_rc)));

		/* set old user context */
		SetUserIdAndSecContext(saved_userid, saved_sec_context);
		user_switched = false;
	}
	PG_CATCH();
	{
		if (user_switched)
		{
			SetUserIdAndSecContext(saved_userid, saved_sec_context);
			user_switched = false;
		}

		PG_RE_THROW();
	}
	PG_END_TRY();

	table_close(relation, AccessShareLock);
	PG_RETURN_INT64((int64)sum_result);
}

/* schema_set_antitamper_rel:
 *    set schema_antitamper relation 
 *    when the mode is set the namespace to antitampering mode
 *    it is not saved in the system relation pg_namespace.
 *    so it is saved in the schema_antitamper relation.
 */
static bool 
schema_set_antitamper_rel(uint32 schemaoid, bool isantitamper)
{
	HeapTuple tup;
	Relation rel;
	Datum values[Natts_schema_antitamper];
	bool nulls[Natts_schema_antitamper];
	Oid schemaantitamperoid = InvalidOid;

	schemaantitamperoid = get_relname_relid(HG_SYS_SCHEMA_ANTITAMPER, SECURITY_SCHEMA_NAMESPACE);
	if (!OidIsValid(schemaantitamperoid))
	{
		return false;
	}
	/* open table schema_antitamper */
	rel = table_open(schemaantitamperoid, RowExclusiveLock);

	/* zero-clear */
	memset(values, 0, sizeof(values));
	memset(nulls, 0, sizeof(nulls));

	values[Anum_schema_antitamper_schemaoid_col - 1] = UInt32GetDatum(schemaoid);
	values[Anum_schema_antitamper_antitamper_col - 1] = BoolGetDatum(isantitamper);

	/* insert the rule attribute into the table. */
	tup = heap_form_tuple(RelationGetDescr(rel), values, nulls);
	simple_heap_insert(rel, tup);
	table_close(rel, RowExclusiveLock);

	return true;
}

/* schema_get_antitamper_flag:
 *    get antitampering mode from schema_antitamper relation 
 * 
 *    when the mode is set the namespace to antitampering mode
 *    it is not saved in the system relation pg_namespace.
 *    so it is saved in the hg_security.schema_antitamper relation.
 */
static bool
schema_get_antitamper_flag(uint32 schemaoid)
{
	HeapTuple tup;
	Relation rel;
	ScanKeyData scanKey[1];
	TableScanDesc scandesc;
	bool is_antitamper = false;
	Datum isantitamper;
	bool isNull;
	Oid schemaantitamperoid = InvalidOid;

	schemaantitamperoid = get_relname_relid(HG_SYS_SCHEMA_ANTITAMPER, SECURITY_SCHEMA_NAMESPACE);
	if (!OidIsValid(schemaantitamperoid))
	{
		return false;
	}
	/* open table schema_antitamper */
	rel = table_open(schemaantitamperoid, AccessShareLock);

	/* Through the user oid,look up records in the table. */
	ScanKeyInit(&scanKey[0],
				1,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(schemaoid));

	scandesc = table_beginscan(rel, SnapshotSelf, 1, scanKey);

	tup = heap_getnext(scandesc, ForwardScanDirection);
	if (HeapTupleIsValid(tup))
	{
		isantitamper = heap_getattr(tup, Anum_schema_antitamper_antitamper_col, RelationGetDescr(rel), &isNull);
		is_antitamper = DatumGetBool(isantitamper);
	}

	table_endscan(scandesc);
	table_close(rel, AccessShareLock);

	return is_antitamper;
}

void
_PG_init(void)
{
	SchemaSetAntiTamperRel = schema_set_antitamper_rel;
	SchemaGetAntiTamperFlag = schema_get_antitamper_flag;
}

void
_PG_fini(void)
{

}
/* End - ReqID: SRS-SECURITY-ANTITAMPERING */
