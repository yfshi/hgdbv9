/*-------------------------------------------------------------------------
 *
 * encrypted_column.c
 *	  Encrypted column processing functions
 *
 * This module handles parsing and validation of ENCRYPTED WITH clauses
 * in CREATE TABLE statements.
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/backend/commands/encrypted_column.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/genam.h"
#include "access/heapam.h"
#include "access/htup_details.h"
#include "access/table.h"
#include "access/sysattr.h"
#include "access/xact.h"
#include "catalog/catalog.h"
#include "catalog/indexing.h"
#include "catalog/namespace.h"
#include "catalog/pg_collation.h"
#include "catalog/pg_database.h"
#include "catalog/pg_type.h"
#include "catalog/hg_kek.h"
#include "catalog/hg_dek.h"
#include "catalog/hg_encrypted_columns.h"
#include "catalog/dependency.h"
#include "commands/defrem.h"
#include "commands/encrypted_column.h"
#include "parser/parse_type.h"
#include "commands/dbcommands.h"
#include "mb/pg_wchar.h"
#include "miscadmin.h"
#include "nodes/makefuncs.h"
#include "parser/parse_utilcmd.h"
#include "nodes/primnodes.h"
#include "nodes/nodeFuncs.h"
#include "utils/acl.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/regproc.h"
#include "utils/syscache.h"
#include "utils/timestamp.h"
#include "utils/hg_key_utils.h"
#include "commands/encrypted_column.h"
#include "utils/tecc_global_mark.h"
#include "utils/tecc/tecc_type_mapping.h"

/*
 * parse_encrypted_with_options - parse ENCRYPTED WITH clause options
 *
 * Returns a pointer to EncryptedColumnParams structure containing parsed
 * parameters, or NULL if parsing fails.
 */
EncryptedColumnParams *
parse_encrypted_with_options(List *options)
{
	EncryptedColumnParams *params;
	ListCell   *lc;
	bool		has_column_encryption_key = false;
	bool		has_encryption_type = false;

	/* Allocate and initialize parameter structure */
	params = (EncryptedColumnParams *) palloc0(sizeof(EncryptedColumnParams));
	params->encryption_type = pstrdup(DEFAULT_ENCRYPTION_TYPE);
	/* Set default encryption type mask (RND) */
	params->encryption_type_mask = 0;

	/* Parse each option */
	foreach(lc, options)
	{
		DefElem    *defel = (DefElem *) lfirst(lc);

		if (strcmp(defel->defname, "dek") == 0)
		{
			if (has_column_encryption_key)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("DEK specified more than once")));

			params->column_encryption_key = defGetString(defel);
			has_column_encryption_key = true;
		}
		else if (strcmp(defel->defname, "support_algo") == 0)
		{
			if (has_encryption_type)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("SUPPORT_ALGO specified more than once")));

			params->encryption_type = defGetString(defel);
			has_encryption_type = true;

			/* Calculate encryption type mask from comma-separated list */
			/* Note: This will be done in calculate_encryption_type_mask function */
		}
		else
		{
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("unrecognized ENCRYPTED WITH option \"%s\"", defel->defname),
					 errhint("Valid options are: DEK, SUPPORT_ALGO")));
		}
	}

	/* Validate required parameters */
	if (!has_column_encryption_key)
	{
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("DEK is required in ENCRYPTED WITH clause")));
	}

	/* Calculate encryption type bitmask */
	params->encryption_type_mask = calculate_encryption_type_mask(params->encryption_type);

	return params;
}

/*
 * is_encrypted_data_type - check if a type OID is encrypted data type
 *
 * Returns true if the type is encrypted data type, false otherwise.
 */
bool
is_encrypted_data_type(Oid type_oid)
{
	return (type_oid >= FIRST_ENCRYPTED_TYPE_OID &&
			type_oid <= LAST_ENCRYPTED_TYPE_OID);
}

/* Begin - ReqID:TECC4.0 */
/*
 * tecc_resolve_collation_oid
 *		Resolve DEFAULT_COLLATION_OID to a concrete collation OID.
 *
 * Encrypted columns must have a fixed collation at creation time, not a
 * "default" placeholder that follows the database default.  If the input
 * is DEFAULT_COLLATION_OID (100), look up the actual database collation
 * from pg_database.datcollate and return its OID.  Otherwise return the
 * input unchanged.
 */
Oid
tecc_resolve_collation_oid(Oid collation_oid)
{
	HeapTuple	dtp;
	Form_pg_database dbform;
	char	   *datcollate;
	Oid			resolved_oid;

	/* Already a concrete collation OID */
	if (collation_oid != DEFAULT_COLLATION_OID)
		return collation_oid;

	/* Look up the database's LC_COLLATE setting */
	dtp = SearchSysCache1(DATABASEOID, ObjectIdGetDatum(MyDatabaseId));
	if (!HeapTupleIsValid(dtp))
		elog(ERROR, "cache lookup failed for database %u", MyDatabaseId);
	dbform = (Form_pg_database) GETSTRUCT(dtp);
	datcollate = pstrdup(NameStr(dbform->datcollate));
	ReleaseSysCache(dtp);

	/*
	 * Scan pg_collation to find the entry whose collcollate matches our
	 * database LC_COLLATE.  Compare case-insensitively and ignoring '-'
	 * characters, because formats differ across platforms
	 * (e.g. "en_US.UTF-8" vs "en_US.utf8").
	 */
	{
		Relation	rel;
		SysScanDesc	scan;
		HeapTuple	ctup;
		Oid			dbencoding = GetDatabaseEncoding();
		char	   *norm_datcollate;

		/* Build normalized copy: lowercase, remove '-' */
		norm_datcollate = pstrdup(datcollate);
		{
			char   *src = norm_datcollate;
			char   *dst = norm_datcollate;

			while (*src)
			{
				char	c = *src++;

				if (c == '-')
					continue;
				if (c >= 'A' && c <= 'Z')
					c += 'a' - 'A';
				*dst++ = c;
			}
			*dst = '\0';
		}

		resolved_oid = InvalidOid;
		rel = table_open(CollationRelationId, AccessShareLock);
		scan = systable_beginscan(rel, InvalidOid, false,
								  NULL, 0, NULL);
		while (HeapTupleIsValid(ctup = systable_getnext(scan)))
		{
			Form_pg_collation collform = (Form_pg_collation) GETSTRUCT(ctup);
			const char *coll = NameStr(collform->collcollate);
			const char *sp;
			bool		match = true;

			/* Filter by encoding */
			if (collform->collencoding != dbencoding &&
				collform->collencoding != -1)
				continue;

			/* Skip ICU collations by provider not 'c' */
			if (collform->collprovider != COLLPROVIDER_DEFAULT &&
				collform->collprovider != COLLPROVIDER_LIBC)
				continue;

			/* Compare normalized */
			sp = coll;
			{
				char   *normp = norm_datcollate;

				while (*normp)
				{
					char	nc = *normp++;

					/* Skip '-' in source */
					while (*sp == '-')
						sp++;

				if (*sp == '\0')
				{
					match = false;
					break;
				}

				{
					char	sc = *sp++;

					if (sc >= 'A' && sc <= 'Z')
						sc += 'a' - 'A';

					if (nc != sc)
					{
						match = false;
						break;
					}
				}
			}
				/* Ensure no trailing '-' in source */
				if (match)
				{
					while (*sp == '-')
						sp++;
					if (*sp != '\0')
						match = false;
				}
			}

			if (match)
			{
				resolved_oid = collform->oid;
				break;
			}
		}
		systable_endscan(scan);
		table_close(rel, AccessShareLock);
		pfree(norm_datcollate);

		if (!OidIsValid(resolved_oid))
		{
			ereport(WARNING,
					(errmsg("could not resolve default collation for "
							"locale \"%s\" (encoding=%d), "
							"collation will follow database default",
							datcollate, dbencoding)));
			resolved_oid = DEFAULT_COLLATION_OID;
		}
	}

	elog(DEBUG1, "tecc_resolve_collation_oid: DEFAULT_COLLATION_OID -> %u (datcollate=%s)",
		 resolved_oid, datcollate);
	pfree(datcollate);

	return resolved_oid;
}
/* End - ReqID:TECC4.0 */

/*
 * validate_encrypted_column_params - validate encrypted column parameters
 *
 * Returns true if parameters are valid, false otherwise.
 */
bool
validate_encrypted_column_params(EncryptedColumnParams *params, Oid column_type)
{
	/* Validate encryption type (support_algo parameter) */
	/* support_algo can be empty (default RND) or comma-separated list of det,ope,search */
	/* ReqID:TECC4.0 - Fix C90 mixed declaration warning - declare variables at function scope beginning */
	char *end = NULL;

	if (params->encryption_type != NULL && strlen(params->encryption_type) > 0)
	{
		/* Validate that the string contains only valid algorithms */
		char *str_copy = pstrdup(params->encryption_type);
		char *token;
		char *saveptr;
		bool has_error = false;

		token = strtok_r(str_copy, ",", &saveptr);
		while (token != NULL)
		{
			/* Trim whitespace */
			while (*token == ' ') token++;
			end = token + strlen(token) - 1;
			while (end > token && *end == ' ') end--;
			*(end + 1) = '\0';

			/* Check each algorithm */
			if (strcasecmp(token, "det") != 0 &&
				strcasecmp(token, "ope") != 0 &&
				strcasecmp(token, "search") != 0)
			{
				has_error = true;
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						 errmsg("invalid algorithm in SUPPORT_ALGO: \"%s\"", token),
						 errhint("Valid algorithms are: det, ope, search")));
				break;
			}

			token = strtok_r(NULL, ",", &saveptr);
		}

		pfree(str_copy);
		if (has_error)
			return false;
	}

	/* Validate column type is encrypted data type */
	/* Skip validation if column_type is InvalidOid (0) - will be validated later */
	if (column_type != InvalidOid && !is_encrypted_data_type(column_type))
	{
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("ENCRYPTED WITH clause can only be used with encrypted data types"),
				 errhint("Use int2_enc, int4_enc, int8_enc, float4_enc, float8_enc, numeric_enc, varchar_enc, bpchar_enc, or text_enc")));
		return false;
	}

	/* ReqID:TECC4.0 - Bug 7999: Numeric encrypted types cannot use SEARCH algorithm */
	if (column_type != InvalidOid &&
		column_type >= INT2_ENCOID && column_type <= NUMERIC_ENCOID &&
		(params->encryption_type_mask & HG_ENCRYPTION_TYPE_SEARCH))
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("SEARCH algorithm is not supported for numeric encrypted types"),
				 errhint("SEARCH (fuzzy matching via LIKE) is only supported for character encrypted types "
				         "(text_enc, varchar_enc, bpchar_enc). Remove 'search' from SUPPORT_ALGO "
				         "for the numeric column, or change the column type to a character encrypted type.")));
		return false;
	}

	/* TODO: Validate DEK exists and user has permission to use it */

	return true;
}

/*
 * calculate_encryption_type_mask - convert encryption type string to bitmask
 *
 * Returns bitmask representation of encryption type.
 */
uint32
calculate_encryption_type_mask(const char *encryption_type_str)
{
	uint32 mask = 0;
	char *str_copy;
	char *token;
	char *saveptr;
	/* ReqID:TECC4.0 - Fix C90 mixed declaration warning - declare variables at function scope beginning */
	char *end = NULL;

	if (encryption_type_str == NULL || strlen(encryption_type_str) == 0)
	{
		/* No support_algo specified — RND only, no DET/OPE/SEARCH.
		 * Equality and range queries will not work without explicit algorithm. */
		return 0;
	}

	/* Make a copy for tokenization */
	str_copy = pstrdup(encryption_type_str);

	/* Parse comma-separated list */
	token = strtok_r(str_copy, ",", &saveptr);
	while (token != NULL)
	{
		/* Trim whitespace */
		while (*token == ' ') token++;
		end = token + strlen(token) - 1;
		while (end > token && *end == ' ') end--;
		*(end + 1) = '\0';

		/* Check each algorithm */
		if (strcasecmp(token, "det") == 0)
			mask |= HG_ENCRYPTION_TYPE_DET;
		else if (strcasecmp(token, "ope") == 0)
			mask |= HG_ENCRYPTION_TYPE_OPE;
		else if (strcasecmp(token, "search") == 0)
			mask |= HG_ENCRYPTION_TYPE_SEARCH;
		else
		{
			/* Unknown algorithm */
			pfree(str_copy);
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("invalid encryption algorithm in SUPPORT_ALGO: \"%s\"", token),
					 errhint("Valid algorithms are: det, ope, search")));
		}

		token = strtok_r(NULL, ",", &saveptr);
	}

	/* DET and OPE algorithms are mutually exclusive */
	if ((mask & HG_ENCRYPTION_TYPE_DET) && (mask & HG_ENCRYPTION_TYPE_OPE))
	{
		pfree(str_copy);
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("DET and OPE algorithms cannot be specified together in SUPPORT_ALGO"),
				 errhint("DET is for equality queries, OPE is for range queries. Choose one.")));
	}

	pfree(str_copy);
	return mask;
}

/*
 * record_encrypted_column_metadata - record encrypted column metadata
 *
 * Records encrypted column metadata in hg_encrypted_columns system table.
 * Returns OID of the new record.
 */
Oid
record_encrypted_column_metadata(Oid table_oid, int16 attnum,
								 const char *column_name, Oid column_type,
								 int32 type_mod, Oid collation_oid,
								 EncryptedColumnParams *params)
{
	HgEncryptedColumnCreateParams hg_params;
	HgMetadataResult result;
	Oid			dek_oid;

	/* Lookup DEK OID by name */
	elog(DEBUG1, "record_encrypted_column_metadata: looking up DEK '%s' for column '%s'",
		 params->column_encryption_key, column_name);
	dek_oid = get_dek_oid(params->column_encryption_key, true);  /* missing_ok = true */
	if (!OidIsValid(dek_oid))
	{
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("DEK \"%s\" not found, cannot create encrypted column \"%s\"",
						params->column_encryption_key, column_name),
				 errhint("Create the DEK first with CREATE DEK in TECC encryption mode (psql -k on).")));
	}
	elog(DEBUG1, "record_encrypted_column_metadata: DEK OID = %u", dek_oid);

	/* Prepare parameters for hg_create_encrypted_column */
	memset(&hg_params, 0, sizeof(hg_params));
	hg_params.table_oid = table_oid;
	hg_params.column_attnum = attnum;
	hg_params.column_name = column_name;
	hg_params.data_type = column_type;
	hg_params.column_key_id = dek_oid;
	hg_params.type_mod = type_mod;

	/* ReqID:TECC4.0 - Set collation OID for ICU sort key */
	hg_params.collation_oid = collation_oid;

	/* Set encryption type bitmask */
	hg_params.encryption_type = params->encryption_type_mask;

	/* Call the metadata function */
	result = hg_create_encrypted_column(&hg_params);

	if (result != HG_METADATA_SUCCESS)
	{
		/* ReqID:TECC4.0 - Permission denied is fatal: prevent creating
		 * a table with encrypted columns the user cannot use. */
		if (result == HG_METADATA_PERMISSION_DENIED)
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					 errmsg("permission denied for DEK \"%s\"", params->column_encryption_key),
					 errdetail("You do not have USAGE privilege on DEK \"%s\".",
							  params->column_encryption_key),
					 errhint("Grant USAGE on the DEK to proceed.")));
		else
			ereport(WARNING,
					(errmsg("Failed to record encrypted column metadata for column \"%s\"", column_name),
					 errdetail("Error: %s", hg_metadata_result_to_string(result))));
		return InvalidOid;
	}

	ereport(DEBUG1,
			(errmsg("Recorded encrypted column metadata for column \"%s\"", column_name),
			 errdetail("Table OID: %u, Column attnum: %d, DEK: %s, Encryption type: %s",
					   table_oid, attnum,
					   params->column_encryption_key,
					   params->encryption_type)));

	/* Return placeholder OID for now */
	return InvalidOid;
}

/*
 * create_encrypted_column_callback - callback function to record encrypted column metadata
 *
 * This function is called after table creation to record encrypted column
 * metadata in hg_encrypted_columns system table.
 */
/* ReqID:TECC4.0 - Comment out unused function to fix compiler warning */
#if 0
static void
create_encrypted_column_callback(Oid table_oid, int16 attnum,
								 const char *column_name, Oid column_type,
								 EncryptedColumnParams *params)
{
	HgEncryptedColumnCreateParams hg_params;
	HgMetadataResult result;

	/* Prepare parameters for hg_create_encrypted_column */
	memset(&hg_params, 0, sizeof(hg_params));
	hg_params.table_oid = table_oid;
	hg_params.column_attnum = attnum;
	hg_params.column_name = column_name;
	hg_params.data_type = column_type;

	/* TODO: Lookup DEK OID by name */
	hg_params.column_key_id = InvalidOid;  /* Placeholder */

	/* Set encryption type bitmask */
	hg_params.encryption_type = params->encryption_type_mask;

	/* Call the metadata function */
	result = hg_create_encrypted_column(&hg_params);

	if (result != HG_METADATA_SUCCESS)
	{
		ereport(WARNING,
				(errmsg("Failed to record encrypted column metadata for column \"%s\"", column_name),
				 errdetail("Error code: %d", result)));
	}
}
#endif

/*
 * add_encrypted_column_to_alist - add encrypted column metadata to after list
 *
 * This function would add a callback to record encrypted column
 * metadata after table creation.
 */
static void
add_encrypted_column_to_alist(Oid table_oid, const char *table_name,
							  ColumnDef *column, EncryptedColumnParams *params)
{
	/* TODO: Implement after-list mechanism for metadata recording */
	elog(DEBUG1, "Would add encrypted column \"%s\" in table \"%s\" to after list",
		 column->colname, table_name);
}

/*
 * process_encrypted_column_during_create - process encrypted column during CREATE TABLE
 *
 * This function is called to process ENCRYPTED WITH clauses.
 * It parses the options, validates them, and schedules metadata recording
 * for after table creation.
 */
void
process_encrypted_column_during_create(Oid table_oid, const char *table_name,
									  ColumnDef *column)
{
	EncryptedColumnParams *params;

	/* Parse encrypted_with options */
	params = parse_encrypted_with_options(column->encrypted_with);

	/* Validate parameters */
	/* Get type OID from type name using LookupTypeNameOid */
	{
		Oid typeOid = LookupTypeNameOid(NULL, column->typeName, true);
		if (!OidIsValid(typeOid))
		{
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_OBJECT),
					 errmsg("type \"%s\" does not exist",
							TypeNameToString(column->typeName))));
		}
		if (!validate_encrypted_column_params(params, typeOid))
		{
			/* Error already reported */
			return;
		}
	}

	/* Schedule metadata recording for after table creation */
	add_encrypted_column_to_alist(table_oid, table_name, column, params);

	ereport(DEBUG1,
			(errmsg("Encrypted column \"%s\" in table \"%s\" processed successfully",
					column->colname, table_name),
			 errdetail("DEK: %s, Encryption type: %s, Table OID: %u",
					   params->column_encryption_key,
					   params->encryption_type,
					   table_oid)));

	/* Note: We need to record metadata after table creation when column attnum is known */
	/* The actual metadata recording will happen via after-list mechanism */
}

/*
 * Helper function: Check if encrypted column exists
 */
static bool
check_encrypted_column_exists(Oid table_oid, int16 column_attnum)
{
	Relation	rel;
	SysScanDesc scan;
	ScanKeyData key[2];
	HeapTuple	tuple;
	bool		exists = false;

	rel = table_open(HgEncryptedColumnsRelationId, AccessShareLock);

	ScanKeyInit(&key[0],
				Anum_hg_encrypted_columns_table_oid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(table_oid));

	ScanKeyInit(&key[1],
				Anum_hg_encrypted_columns_column_attnum,
				BTEqualStrategyNumber, F_INT2EQ,
				Int16GetDatum(column_attnum));

	scan = systable_beginscan(rel, HgEncryptedColumnsPkeyIndexId, true,
							  NULL, 2, key);

	tuple = systable_getnext(scan);
	if (HeapTupleIsValid(tuple))
		exists = true;

	systable_endscan(scan);
	table_close(rel, AccessShareLock);

	return exists;
}

/*
 * hg_create_encrypted_column - Create a new encrypted column entry
 *
 * ReqID:TECC4.0 - Update to use DEK OID instead of name
 * Modified by <wangxp@highgo.com>, 20260306.
 */
HgMetadataResult
hg_create_encrypted_column(const HgEncryptedColumnCreateParams *params)
{
	Relation	rel;
	HeapTuple	tuple;
	Datum		values[Natts_hg_encrypted_columns];
	bool		nulls[Natts_hg_encrypted_columns];
	Oid			user_id;
	/* ReqID:TECC4.0 - Fix C90 mixed declaration warning - declare variables at function scope beginning */
	Oid			encrypted_column_oid = InvalidOid;

	/* Validate parameters */
	if (!params || !params->column_name)
		return HG_METADATA_INVALID_PARAM;

	/* Check if column already encrypted */
	if (check_encrypted_column_exists(params->table_oid, params->column_attnum))
		return HG_METADATA_CONSTRAINT_VIOLATION;

	/* Check if referenced DEK exists */
	if (!dek_exists(params->column_key_id))
		return HG_METADATA_NOT_FOUND;

	/* Get current user ID */
	user_id = GetUserId();

	/* Check permission - need permission on the DEK */
	if ((hg_dek_aclmask(params->column_key_id, user_id, ACL_USAGE, ACLMASK_ANY) & ACL_USAGE) == 0)
		return HG_METADATA_PERMISSION_DENIED;

	/* Open relation */
	rel = table_open(HgEncryptedColumnsRelationId, RowExclusiveLock);

	/* Prepare tuple data */
	memset(values, 0, sizeof(values));
	memset(nulls, false, sizeof(nulls));

	/* Generate a new OID for the encrypted column record */
	encrypted_column_oid = GetNewOidWithIndex(rel, HgEncryptedColumnsOidIndexId,
											  Anum_hg_encrypted_columns_oid);

	/* Debug: log all parameters */
	elog(DEBUG1, "hg_create_encrypted_column: table_oid=%u, column_attnum=%d, column_name='%s', data_type=%u, column_key_id=%u, encryption_type=%u",
		 params->table_oid, params->column_attnum, params->column_name, params->data_type, params->column_key_id, params->encryption_type);

	/* encryption_type - calculate from params */
	{
		uint32 encryption_mask = 0;

		/* If encryption_type field is provided, use it */
		if (params->encryption_type != 0)
		{
			encryption_mask = params->encryption_type;
		}
		/* Otherwise, calculate from boolean fields (backward compatibility) */
		else
		{
			if (params->enable_det)
				encryption_mask |= HG_ENCRYPTION_TYPE_DET;
			if (params->enable_ope)
				encryption_mask |= HG_ENCRYPTION_TYPE_OPE;
			if (params->enable_search)
				encryption_mask |= HG_ENCRYPTION_TYPE_SEARCH;

			/* Default to OPE if no encryption type specified */
			if (encryption_mask == 0)
				encryption_mask = 0;
		}

		values[Anum_hg_encrypted_columns_encryption_type - 1] = Int32GetDatum(encryption_mask);
	}

	/* table_oid */
	values[Anum_hg_encrypted_columns_table_oid - 1] = ObjectIdGetDatum(params->table_oid);

	/* column_attnum */
	values[Anum_hg_encrypted_columns_column_attnum - 1] = Int16GetDatum(params->column_attnum);

	/* column_name */
	values[Anum_hg_encrypted_columns_column_name - 1] = DirectFunctionCall1(namein,
																		   CStringGetDatum(params->column_name));

	/* data_type_original_oid - convert encrypted type OID to base type OID */
	values[Anum_hg_encrypted_columns_data_type_original_oid - 1] = ObjectIdGetDatum(get_base_type_from_encrypted(params->data_type));

	/* data_type_original_mod - store type modifier (typmod) for frontend validation */
	values[Anum_hg_encrypted_columns_data_type_original_mod - 1] = Int32GetDatum(params->type_mod);

	/* column_key_id */
	values[Anum_hg_encrypted_columns_column_key_id - 1] = ObjectIdGetDatum(params->column_key_id);

	/* encryption_type - calculate from params */
	{
		uint32 encryption_mask = 0;

		/* If encryption_type field is provided, use it */
		if (params->encryption_type != 0)
		{
			encryption_mask = params->encryption_type;
		}
		/* Otherwise, calculate from boolean fields (backward compatibility) */
		else
		{
			if (params->enable_det)
				encryption_mask |= HG_ENCRYPTION_TYPE_DET;
			if (params->enable_ope)
				encryption_mask |= HG_ENCRYPTION_TYPE_OPE;
			if (params->enable_search)
				encryption_mask |= HG_ENCRYPTION_TYPE_SEARCH;

			/* Default to OPE if no encryption type specified */
			if (encryption_mask == 0)
				encryption_mask = 0;
		}

		values[Anum_hg_encrypted_columns_encryption_type - 1] = Int32GetDatum(encryption_mask);
	}

	/* ReqID:TECC4.0 - Use collation_oid from params for ICU sort key support */
	values[Anum_hg_encrypted_columns_collation_oid - 1] = ObjectIdGetDatum(params->collation_oid);

	/* create_date - set to current timestamp */
	values[Anum_hg_encrypted_columns_create_date - 1] = TimestampTzGetDatum(GetCurrentTimestamp());

	/* Set the OID for this encrypted column record */
	values[Anum_hg_encrypted_columns_oid - 1] = ObjectIdGetDatum(encrypted_column_oid);

	/* Create and insert tuple */
	tuple = heap_form_tuple(RelationGetDescr(rel), values, nulls);

	CatalogTupleInsert(rel, tuple);

	/* Record dependency: encrypted column -> table column (AUTO) */
	{
		ObjectAddress encrypted_column_addr;
		ObjectAddress table_column_addr;

		/* Set up encrypted column object address */
		encrypted_column_addr.classId = HgEncryptedColumnsRelationId;
		encrypted_column_addr.objectId = encrypted_column_oid;
		encrypted_column_addr.objectSubId = 0;

		/* Encrypted column depends on table column (AUTO) */
		table_column_addr.classId = RelationRelationId;
		table_column_addr.objectId = params->table_oid;
		table_column_addr.objectSubId = params->column_attnum;

		recordDependencyOn(&encrypted_column_addr, &table_column_addr,
					   DEPENDENCY_AUTO);
	}
	/* Mark column metadata as updated */
	/* tecc_mark_column_updated(); */

	/* Clean up */
	heap_freetuple(tuple);
	table_close(rel, RowExclusiveLock);

	return HG_METADATA_SUCCESS;
}

/*
 * Validation function for encrypted column parameters
 */
bool
hg_validate_encrypted_column_params(const HgEncryptedColumnCreateParams *params)
{
	if (!params)
		return false;

	/* Check column name length */
	if (strlen(params->column_name) >= NAMEDATALEN)
		return false;

	/* Check column key OID is valid */
	if (!OidIsValid(params->column_key_id))
		return false;

	/* With new design: RND is default, support_algo can contain det, ope, search in any combination */
	/* No specific algorithm is required - all are optional */
	/* The encryption_type field should contain valid bitmask if provided */

	return true;
}

/*
 * Utility function: Convert metadata result to string
 */
const char *
hg_metadata_result_to_string(HgMetadataResult result)
{
	switch (result)
	{
		case HG_METADATA_SUCCESS:
			return "Success";
		case HG_METADATA_ERROR:
			return "General error";
		case HG_METADATA_NOT_FOUND:
			return "Not found";
		case HG_METADATA_PERMISSION_DENIED:
			return "Permission denied";
		case HG_METADATA_INVALID_PARAM:
			return "Invalid parameter";
		case HG_METADATA_CONSTRAINT_VIOLATION:
			return "Constraint violation";
		default:
			return "Unknown error";
	}
}
/* ReqID:TECC4.0 - CTAS encryption metadata propagation */

/*
 * hg_get_encrypted_column_info - read encryption metadata from source column
 *
 * Look up a column's encryption metadata in hg_encrypted_columns by
 * (table_oid, column_attnum) primary key.  Returns the DEK OID and
 * encryption type bitmask if found.
 *
 * Returns true if an entry exists, false otherwise.
 */
bool
hg_get_encrypted_column_info(Oid table_oid, int16 attnum,
							 Oid *dek_oid, uint32 *encryption_type_mask)
{
	Relation	rel;
	SysScanDesc scan;
	ScanKeyData key[2];
	HeapTuple	tuple;
	bool		found = false;

	rel = table_open(HgEncryptedColumnsRelationId, AccessShareLock);

	ScanKeyInit(&key[0],
				Anum_hg_encrypted_columns_table_oid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(table_oid));

	ScanKeyInit(&key[1],
				Anum_hg_encrypted_columns_column_attnum,
				BTEqualStrategyNumber, F_INT2EQ,
				Int16GetDatum(attnum));

	scan = systable_beginscan(rel, HgEncryptedColumnsPkeyIndexId, true,
							  NULL, 2, key);

	tuple = systable_getnext(scan);
	if (HeapTupleIsValid(tuple))
	{
		Form_hg_encrypted_columns form;
		form = (Form_hg_encrypted_columns) GETSTRUCT(tuple);
		if (dek_oid)
			*dek_oid = form->column_key_id;
		if (encryption_type_mask)
			*encryption_type_mask = (uint32) form->encryption_type;
		found = true;
	}

	systable_endscan(scan);
	table_close(rel, AccessShareLock);

	return found;
}

/*
 * find_first_var_walker - expression_tree_walker callback
 *
 * Walks an expression tree looking for the first Var node.
 * Stops immediately when one is found.
 */
typedef struct FindFirstVarContext
{
	Var *result;
} FindFirstVarContext;

static bool
find_first_var_walker(Node *node, FindFirstVarContext *context)
{
	if (node == NULL)
		return false;
	if (IsA(node, Var))
	{
		context->result = (Var *) node;
		return true;	/* found, stop walk */
	}
	return expression_tree_walker(node, find_first_var_walker, context);
}

/*
 * find_first_var_in_expr - find the first Var node in an expression tree
 *
 * Returns the first Var found, or NULL if none exists.
 */
static Var *
find_first_var_in_expr(Node *node)
{
	FindFirstVarContext context;

	context.result = NULL;
	find_first_var_walker(node, &context);
	return context.result;
}

/*
 * populate_ctas_encryption_metadata
 *
 * After a CTAS table is created, populate hg_encrypted_columns entries
 * for columns with encrypted data types.  Traces each encrypted column
 * back to its source table/column through the plan's target list and
 * range table, then copies the encryption metadata.
 */
void
populate_ctas_encryption_metadata(Oid new_table_oid, List *attrList,
								  List *source_tlist, List *rtable)
{
	ListCell   *lc_attr;
	ListCell   *lc_tle;
	int			col_idx;
	bool		has_encrypted;

	if (attrList == NIL || source_tlist == NIL || !OidIsValid(new_table_oid))
		return;

	/* First pass: check if any column is encrypted */
	has_encrypted = false;
	foreach(lc_attr, attrList)
	{
		ColumnDef  *col;
		Oid			typeOid;

		col = (ColumnDef *) lfirst(lc_attr);
		typenameTypeIdAndMod(NULL, col->typeName, &typeOid, NULL);
		if (is_encrypted_data_type(typeOid))
		{
			has_encrypted = true;
			break;
		}
	}

	if (!has_encrypted)
		return;

	/*
	 * attrList[N] corresponds to the N-th non-resjunk TargetEntry in
	 * source_tlist.  Walk both lists in lock-step, skipping resjunk
	 * entries in the target list.
	 */
	col_idx = 0;
	lc_tle = list_head(source_tlist);

	foreach(lc_attr, attrList)
	{
		ColumnDef  *col;
		Oid			typeOid;
		int32		typmod;

		col = (ColumnDef *) lfirst(lc_attr);
		typenameTypeIdAndMod(NULL, col->typeName, &typeOid, &typmod);

		if (is_encrypted_data_type(typeOid))
		{
			/* Skip resjunk TLEs to stay aligned */
			while (lc_tle != NULL &&
				   ((TargetEntry *) lfirst(lc_tle))->resjunk)
				lc_tle = lnext(source_tlist, lc_tle);

			/* ReqID:TECC4.0 - skip columns already registered via
			 * explicit ENCRYPTED WITH in column definition (provided
			 * by frontend CTAS rewrite). */
			if (col->encrypted_with != NIL)
			{
				if (lc_tle != NULL)
					lc_tle = lnext(source_tlist, lc_tle);
				col_idx++;
				continue;
			}

			if (lc_tle != NULL)
			{
				TargetEntry *tle;
				Node	   *expr;
				Var		   *var;
				Oid			src_dek_oid;
				uint32		src_enc_type;
				HgEncryptedColumnCreateParams hg_params;
				int16		attnum;

				tle = (TargetEntry *) lfirst(lc_tle);
				expr = (Node *) tle->expr;
				var = NULL;
				src_dek_oid = InvalidOid;
				src_enc_type = 0;

				if (IsA(expr, Var))
					var = (Var *) expr;
				else
					var = find_first_var_in_expr(expr);

				if (var != NULL &&
					var->varno > 0 &&
					var->varno <= list_length(rtable))
				{
					RangeTblEntry *rte;

					rte = (RangeTblEntry *) list_nth(rtable, var->varno - 1);
					if (rte->rtekind == RTE_RELATION && OidIsValid(rte->relid))
					{
						hg_get_encrypted_column_info(rte->relid,
													 var->varattno,
													 &src_dek_oid,
													 &src_enc_type);
					}
				}

				if (OidIsValid(src_dek_oid))
				{
					attnum = col_idx + 1;	/* 1-based column number */

					memset(&hg_params, 0, sizeof(hg_params));
					hg_params.table_oid = new_table_oid;
					hg_params.column_attnum = attnum;
					hg_params.column_name = col->colname;
					hg_params.data_type = typeOid;
					hg_params.column_key_id = src_dek_oid;
					hg_params.encryption_type = src_enc_type;
					hg_params.type_mod = typmod;

					hg_create_encrypted_column(&hg_params);
				}
				else
				{
					ereport(WARNING,
							(errmsg("could not determine encryption metadata for column \"%s\" in CTAS result table",
									col->colname),
							 errdetail("The column will have encrypted type but no encryption metadata. "
									   "Consider creating the table with an explicit ENCRYPTED WITH clause.")));
				}

				/* Advance to next non-resjunk TLE */
				lc_tle = lnext(source_tlist, lc_tle);
			}
		}
		else
		{
			/* Non-encrypted column: advance past resjunk entries */
			while (lc_tle != NULL &&
				   ((TargetEntry *) lfirst(lc_tle))->resjunk)
				lc_tle = lnext(source_tlist, lc_tle);
			if (lc_tle != NULL)
				lc_tle = lnext(source_tlist, lc_tle);
		}

		col_idx++;
	}
}
