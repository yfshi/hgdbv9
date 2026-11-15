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
#include "catalog/hg_encrypted_column_args.h"
#include "utils/tecc_conn_type.h"
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
#include "utils/pg_locale.h"
#include "utils/regproc.h"
#include "utils/syscache.h"
#include "utils/timestamp.h"
#include "utils/hg_key_utils.h"
#include "commands/encrypted_column.h"
#include "utils/tecc_global_mark.h"
#include "utils/tecc/tecc_type_mapping.h"
/* ReqID:TECC4.0 - For ExecConstraints HE slot validation */
#include "utils/cipherdatatype.h"
#include "executor/tuptable.h"
#include "nodes/execnodes.h"

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
		/* ReqID:TECC4.0 - 一列多 DEK：he-add/he-mul 各自的同态 DEK。
		 * 保存 DEK 名供权威校验（存在性/key_algo/权限）；存储见 P0-2。 */
		else if (strcmp(defel->defname, "he_add_dek") == 0)
		{
			params->he_add_dek = defGetString(defel);
		}
		else if (strcmp(defel->defname, "he_mul_dek") == 0)
		{
			params->he_mul_dek = defGetString(defel);
		}
		else
		{
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("unrecognized ENCRYPTED WITH option \"%s\"", defel->defname),
					 errhint("Valid options are: DEK, SUPPORT_ALGO, HE_ADD_DEK, HE_MUL_DEK")));
		}
	}

	/* Validate required parameters */
	/* ReqID:TECC4.0 - 纯 HE 列（he-add/he-mul）只需 HE_ADD_DEK/HE_MUL_DEK，
	 * 不需要对称 DEK 参数；否则必须提供 DEK。 */
	if (!has_column_encryption_key &&
		params->he_add_dek == NULL && params->he_mul_dek == NULL)
	{
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("DEK is required in ENCRYPTED WITH clause")));
	}

	/* ReqID:TECC4.0 - A14: reject explicit empty SUPPORT_ALGO (e.g. SUPPORT_ALGO='').
	 * Default empty string (no SUPPORT_ALGO specified) is valid — means RND only. */
	if (has_encryption_type &&
		params->encryption_type != NULL && strlen(params->encryption_type) == 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("SUPPORT_ALGO cannot be empty"),
				 errhint("SUPPORT_ALGO must be a comma-separated list of: det, ope, search, he-add, he-mul")));
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
			ereport(ERROR,
					(errmsg("could not resolve default collation OID for "
							"locale \"%s\" (encoding=%d)",
							datcollate, dbencoding)));
	}
	pfree(datcollate);

	elog(DEBUG1, "tecc_resolve_collation_oid: DEFAULT_COLLATION_OID -> %u (datcollate=%s)",
		 resolved_oid, NameStr(dbform->datcollate));

	return resolved_oid;
}
/* End - ReqID:TECC4.0 */

/*
 * tecc_validate_collation_for_encrypted_column - validate collation/encoding
 *			for an encrypted column at CREATE/ALTER TABLE time.
 *
 * OPE encrypted columns rely on client-side ICU sort keys so the collation
 * and database encoding must be usable with ICU.  C/POSIX collations are
 * fine (the client uses the binary/byte-order path without ICU).
 * SEARCH/RND/DET/HE columns do not depend on collation ordering and are
 * not checked here.
 *
 * Fails fast at DDL time instead of producing silently wrong OPE results
 * (or failing later at INSERT time) when the collation cannot be handled.
 */
/* Begin - ReqID:TECC4.0 */
void
tecc_validate_collation_for_encrypted_column(Oid collation_oid,
											 uint32 enc_type_mask)
{
	/* Only OPE depends on collation ordering (ICU sort keys).  SEARCH does
	 * byte-level prefix/exact encryption and needs no collation; RND/DET/HE
	 * do not depend on ordering either. */
	if ((enc_type_mask & HG_ENCRYPTION_TYPE_OPE) == 0)
		return;

#ifdef USE_ICU
	/* Database encoding must be supported by ICU (UTF8 etc.) */
	if (!is_encoding_supported_by_icu(GetDatabaseEncoding()))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("encrypted column with OPE requires an ICU-supported database encoding"),
				 errdetail("Database encoding \"%s\" is not supported by ICU. Recreate the database with an ICU-supported encoding (e.g. UTF8).",
						   pg_encoding_to_char(GetDatabaseEncoding()))));

	/* No collation or C/POSIX: client uses the binary/byte-order path */
	if (!OidIsValid(collation_oid) || lc_collate_is_c(collation_oid))
		return;

	/* Other collations must be openable by ICU (client ucol_open must work) */
	{
		HeapTuple	tp;
		Form_pg_collation collform;
		const char *collcollate;
		UErrorCode	status;
		UCollator  *col;

		tp = SearchSysCache1(COLLOID, ObjectIdGetDatum(collation_oid));
		if (!HeapTupleIsValid(tp))
			elog(ERROR, "cache lookup failed for collation %u", collation_oid);
		collform = (Form_pg_collation) GETSTRUCT(tp);
		collcollate = NameStr(collform->collcollate);

		status = U_ZERO_ERROR;
		col = ucol_open(collcollate, &status);
		if (U_FAILURE(status) || col == NULL)
		{
			char	   *collname = pstrdup(NameStr(collform->collname));

			ReleaseSysCache(tp);
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("collation \"%s\" is not supported by ICU for encrypted column",
							collname),
					 errdetail("OPE encrypted columns require a collation usable with ICU (e.g. C/POSIX or an ICU collation).")));
		}
		ucol_close(col);
		ReleaseSysCache(tp);
	}
#else
	/* Build without ICU: ordering-dependent encrypted columns unavailable */
	ereport(ERROR,
			(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
			 errmsg("encrypted column with OPE requires ICU support"),
			 errdetail("This build was compiled without ICU support.")));
#endif
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
		bool has_token = false;

		token = strtok_r(str_copy, ",", &saveptr);
		while (token != NULL)
		{
			/* Trim whitespace */
			while (*token == ' ') token++;
			end = token + strlen(token) - 1;
			while (end > token && *end == ' ') end--;
			*(end + 1) = '\0';

			/* ReqID:TECC4.0 - A14: reject empty tokens (e.g. SUPPORT_ALGO=',' or ',det') */
			if (strlen(token) == 0)
			{
				pfree(str_copy);
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						 errmsg("SUPPORT_ALGO contains empty algorithm name"),
						 errhint("SUPPORT_ALGO must be a comma-separated list of: det, ope, search, he-add, he-mul")));
			}

			has_token = true;

			/* Check each algorithm */
			if (strcasecmp(token, "det") != 0 &&
				strcasecmp(token, "ope") != 0 &&
				strcasecmp(token, "search") != 0 &&
				strcasecmp(token, "he-add") != 0 &&
				strcasecmp(token, "he-mul") != 0)
			{
				has_error = true;
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						 errmsg("invalid algorithm in SUPPORT_ALGO: \"%s\"", token),
						 errhint("Valid algorithms are: det, ope, search, he-add, he-mul")));
				break;
			}

			token = strtok_r(NULL, ",", &saveptr);
		}

		pfree(str_copy);
		if (has_error)
			return false;

		/* ReqID:TECC4.0 - A14: all-comma string like "," produces no tokens
		 * (strtok_r skips leading delimiters) — reject it. */
		if (!has_token)
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("SUPPORT_ALGO \"%s\" contains no valid algorithm names",
							params->encryption_type),
					 errhint("SUPPORT_ALGO must be a comma-separated list of: det, ope, search, he-add, he-mul")));
		}
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

	/* ReqID:TECC4.0 - A14: HE algorithms (he-add/he-mul) only support integer encrypted types */
	if (column_type != InvalidOid &&
		(params->encryption_type_mask & (HG_ENCRYPTION_TYPE_HE_ADD | HG_ENCRYPTION_TYPE_HE_MUL)) &&
		column_type != INT2_ENCOID && column_type != INT4_ENCOID && column_type != INT8_ENCOID)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("homomorphic encryption (he-add/he-mul) only supports integer encrypted types"),
				 errhint("Use int2_enc, int4_enc, or int8_enc with HE algorithms. "
				         "Remove 'he-add'/'he-mul' from SUPPORT_ALGO for non-integer types.")));
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
		/* Begin - ReqID:TECC4.0 */
		else if (strcasecmp(token, "he-add") == 0)
			mask |= HG_ENCRYPTION_TYPE_HE_ADD;
		else if (strcasecmp(token, "he-mul") == 0)
			mask |= HG_ENCRYPTION_TYPE_HE_MUL;
		/* End - ReqID:TECC4.0 */
		else
		{
			/* Unknown algorithm */
			pfree(str_copy);
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("invalid encryption algorithm in SUPPORT_ALGO: \"%s\"", token),
					 errhint("Valid algorithms are: det, ope, search, he-add, he-mul")));
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
	/* ReqID:TECC4.0 - P0-2 一列多 DEK：he-add/he-mul DEK OID（InvalidOid=无） */
	Oid			hg_params_he_add_dek_oid = InvalidOid;
	Oid			hg_params_he_mul_dek_oid = InvalidOid;

	/*
	 * ReqID:TECC4.0 - pg_upgrade binary upgrade 模式：跳过 DEK 存在性校验
	 * 与元数据记录。升级恢复阶段 hg_dek/hg_encrypted_columns 的行经
	 * pg_upgrade 文件拷贝转移，CREATE TABLE DDL 执行时 DEK 行可能尚不可见；
	 * 元数据行也已在转移的文件中，重复记录会产生重复行。
	 */
	if (tecc_is_binary_upgrade())
		return InvalidOid;

	/* Lookup DEK OID by name（纯 HE 列无对称 DEK，column_key_id 置 InvalidOid） */
	if (params->column_encryption_key != NULL)
	{
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
	}
	else
		dek_oid = InvalidOid;

	/* ReqID:TECC4.0 - P1-1 一列多 DEK 权威校验：he-add/he-mul DEK 的
	 * 存在性、key_algo 匹配、USAGE 权限。防止绕过前端（JDBC 直连）
	 * 提交非法定义。存储见 P0-2（hg_encrypted_column_args）。 */
	if (params->he_add_dek != NULL || params->he_mul_dek != NULL)
	{
		const char *dek_names[2] = {params->he_add_dek, params->he_mul_dek};
		const char *arg_names[2] = {"HE_ADD_DEK", "HE_MUL_DEK"};
		const char *exp_algos[2] = {"he-add", "he-mul"};
		Oid			user_id = GetUserId();
		int			i;

		for (i = 0; i < 2; i++)
		{
			Oid		he_dek_oid;
			char   *key_algo;

			if (dek_names[i] == NULL)
				continue;

			/* 1. 存在性 */
			he_dek_oid = get_dek_oid(dek_names[i], true);
			if (!OidIsValid(he_dek_oid))
				ereport(ERROR,
						(errcode(ERRCODE_UNDEFINED_OBJECT),
						 errmsg("%s \"%s\" not found, cannot create encrypted column \"%s\"",
								arg_names[i], dek_names[i], column_name),
						 errhint("Create the DEK first with CREATE DEK in TECC encryption mode (psql -k on).")));

			/* 2. key_algo 匹配 */
			key_algo = get_dek_arg_value(he_dek_oid, "key_algo");
			if (key_algo != NULL)
			{
				bool	algo_ok = (strcmp(key_algo, exp_algos[i]) == 0);
				pfree(key_algo);

				if (!algo_ok)
					ereport(ERROR,
							(errcode(ERRCODE_TECC_ALGORITHM_INCOMPATIBLE),
							 errmsg("%s \"%s\" for column \"%s\" must be a %s DEK",
									arg_names[i], dek_names[i], column_name, exp_algos[i])));
			}
			else
				ereport(ERROR,
						(errcode(ERRCODE_TECC_ALGORITHM_INCOMPATIBLE),
						 errmsg("%s \"%s\" for column \"%s\" must be a %s DEK",
								arg_names[i], dek_names[i], column_name, exp_algos[i])));

			/* 3. USAGE 权限 */
			if ((hg_dek_aclmask(he_dek_oid, user_id, ACL_USAGE, ACLMASK_ANY) & ACL_USAGE) == 0)
				ereport(ERROR,
						(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
						 errmsg("permission denied for DEK \"%s\"", dek_names[i]),
						 errdetail("You do not have USAGE privilege on DEK \"%s\".", dek_names[i]),
						 errhint("Grant USAGE on the DEK to proceed.")));

			/* ReqID:TECC4.0 - P0-2 保存 he-add/he-mul DEK OID 供 args 写入 */
			if (i == 0)
				hg_params_he_add_dek_oid = he_dek_oid;
			else
				hg_params_he_mul_dek_oid = he_dek_oid;
		}
	}

	/* Prepare parameters for hg_create_encrypted_column */
	memset(&hg_params, 0, sizeof(hg_params));
	hg_params.table_oid = table_oid;
	hg_params.column_attnum = attnum;
	hg_params.column_name = column_name;
	hg_params.data_type = column_type;
	hg_params.column_key_id = dek_oid;
	hg_params.type_mod = type_mod;

	/* ReqID:TECC4.0 - P0-2 一列多 DEK：he-add/he-mul DEK OID */
	hg_params.he_add_dek_id = hg_params_he_add_dek_oid;
	hg_params.he_mul_dek_id = hg_params_he_mul_dek_oid;

	/* ReqID:TECC4.0 - Set collation OID for ICU sort key */
	hg_params.collation_oid = collation_oid;

	/* ReqID:TECC4.0 - 纯 HE 列（仅 HE_ADD_DEK/HE_MUL_DEK、无 SUPPORT_ALGO）时
	 * encryption_type 需含 HE_ADD/HE_MUL 位，否则前端 enable_he_add 判定失败
	 * 会误走 RND 加密（INSERT 产生 RND 段、HE 段为空）。 */
	if (hg_params_he_add_dek_oid != InvalidOid)
		params->encryption_type_mask |= HG_ENCRYPTION_TYPE_HE_ADD;
	if (hg_params_he_mul_dek_oid != InvalidOid)
		params->encryption_type_mask |= HG_ENCRYPTION_TYPE_HE_MUL;

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
 * process_encrypted_column_during_create - process encrypted column during CREATE TABLE
 *
 * This function is called to process ENCRYPTED WITH clauses.
 * It parses the options and validates them.
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
 * get_dek_distributed_id - 按 DEK OID 查其分布式身份标识
 * (hg_dek.column_key_distributed_id)。用于 hg_encrypted_column_args 的
 * value 存储（跨实例确定性的 DEK 身份，与密文段内嵌 DEK ID 同一体系）。
 */
static Oid
get_dek_distributed_id(Oid dek_oid)
{
	Relation	rel;
	SysScanDesc scan;
	ScanKeyData key[1];
	HeapTuple	tup;
	Oid			dist_id = InvalidOid;

	if (!OidIsValid(dek_oid))
		return InvalidOid;

	rel = table_open(HgDekRelationId, AccessShareLock);
	ScanKeyInit(&key[0], Anum_hg_dek_oid,
				BTEqualStrategyNumber, F_OIDEQ, ObjectIdGetDatum(dek_oid));
	scan = systable_beginscan(rel, HgDekOidIndexId, true, NULL, 1, key);

	tup = systable_getnext(scan);
	if (HeapTupleIsValid(tup))
	{
		Form_hg_dek form = (Form_hg_dek) GETSTRUCT(tup);
		dist_id = form->column_key_distributed_id;
	}
	systable_endscan(scan);
	table_close(rel, AccessShareLock);

	return dist_id;
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

	/* Check if referenced DEK exists（纯 HE 列无对称 DEK，跳过） */
	if (OidIsValid(params->column_key_id) && !dek_exists(params->column_key_id))
		return HG_METADATA_NOT_FOUND;

	/* Get current user ID */
	user_id = GetUserId();

	/* Check permission - need permission on the DEK（纯 HE 列无对称 DEK，跳过） */
	if (OidIsValid(params->column_key_id) &&
		(hg_dek_aclmask(params->column_key_id, user_id, ACL_USAGE, ACLMASK_ANY) & ACL_USAGE) == 0)
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

	/* ReqID:TECC4.0 - P0-2 一列多 DEK：写 he-add/he-mul DEK 绑定到
	 * hg_encrypted_column_args（value 为 DEK 分布式身份标识）。对称 DEK
	 * 由 column_key_id 承载，不写 args。 */
	if (OidIsValid(params->he_add_dek_id) || OidIsValid(params->he_mul_dek_id))
	{
		Relation	args_rel;
		Oid			he_dek_ids[2] = {params->he_add_dek_id, params->he_mul_dek_id};
		const char *he_arg_keys[2] = {"he_add_dek", "he_mul_dek"};
		int			ai;

		args_rel = table_open(HgEncryptedColumnArgsRelationId, RowExclusiveLock);
		for (ai = 0; ai < 2; ai++)
		{
			Datum		args_values[Natts_hg_encrypted_column_args];
			bool		args_nulls[Natts_hg_encrypted_column_args];
			HeapTuple	args_tuple;
			Oid			args_oid;

			if (!OidIsValid(he_dek_ids[ai]))
				continue;

			memset(args_values, 0, sizeof(args_values));
			memset(args_nulls, false, sizeof(args_nulls));

			args_oid = GetNewOidWithIndex(args_rel, HgEncryptedColumnArgsOidIndexId,
										  Anum_hg_encrypted_column_args_oid);
			args_values[Anum_hg_encrypted_column_args_oid - 1] = ObjectIdGetDatum(args_oid);
			args_values[Anum_hg_encrypted_column_args_column_id - 1] = ObjectIdGetDatum(encrypted_column_oid);
			args_values[Anum_hg_encrypted_column_args_function_name - 1] =
				DirectFunctionCall1(namein, CStringGetDatum("kek_config"));
			args_values[Anum_hg_encrypted_column_args_key - 1] =
				DirectFunctionCall1(namein, CStringGetDatum(he_arg_keys[ai]));

			/* value = DEK 分布式身份标识（column_key_distributed_id），
			 * 以十进制文本存入 bytea，视图用 value::text::int4 读回。 */
			{
				Oid		dist_id = get_dek_distributed_id(he_dek_ids[ai]);
				char   *dist_str = psprintf("%u", (unsigned int) dist_id);

				args_values[Anum_hg_encrypted_column_args_value - 1] =
					PointerGetDatum(cstring_to_text_with_len(dist_str, strlen(dist_str)));
			}

			args_tuple = heap_form_tuple(RelationGetDescr(args_rel), args_values, args_nulls);
			CatalogTupleInsert(args_rel, args_tuple);

			/* ReqID:TECC4.0 - Record dependency: args entry -> encrypted
			 * column (AUTO), so DROP TABLE/column cascades to delete the
			 * args entries (prevents orphan rows in hg_encrypted_column_args). */
			{
				ObjectAddress args_addr;
				ObjectAddress encrypted_column_addr;

				args_addr.classId = HgEncryptedColumnArgsRelationId;
				args_addr.objectId = args_oid;
				args_addr.objectSubId = 0;

				encrypted_column_addr.classId = HgEncryptedColumnsRelationId;
				encrypted_column_addr.objectId = encrypted_column_oid;
				encrypted_column_addr.objectSubId = 0;

				recordDependencyOn(&args_addr, &encrypted_column_addr,
								   DEPENDENCY_AUTO);
			}

			heap_freetuple(args_tuple);
		}
		table_close(args_rel, RowExclusiveLock);
	}

	/* Clean up */
	heap_freetuple(tuple);
	table_close(rel, RowExclusiveLock);

	return HG_METADATA_SUCCESS;
}

/* Begin - ReqID:TECC4.0 - PARTITION OF：密态列元数据整份继承 */
/*
 * hg_copy_encrypted_column_args
 *		把 src_column_id 在 hg_encrypted_column_args 里的行整行复制到
 *		dst_column_id 名下（只换 oid 与 column_id），返回复制行数。
 *
 *		he_add_dek / he_mul_dek 绑定存在这里，值是该 HE DEK 的分布式身份标识。
 *		整行照抄而不是"重新解析成 DEK 名再写回"，就不会因为 value 的编码形态
 *		变化而抄错，也自动覆盖以后新增的 arg 键。
 */
static int
hg_copy_encrypted_column_args(Oid src_column_id, Oid dst_column_id)
{
	Relation	args_rel;
	SysScanDesc args_scan;
	ScanKeyData args_key;
	HeapTuple	args_tuple;
	int			copied = 0;

	if (!OidIsValid(src_column_id) || !OidIsValid(dst_column_id))
		return 0;

	args_rel = table_open(HgEncryptedColumnArgsRelationId, RowExclusiveLock);

	/* 主键是 (column_id, key)，只给 column_id 就是一次前缀扫描 */
	ScanKeyInit(&args_key,
				Anum_hg_encrypted_column_args_column_id,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(src_column_id));
	args_scan = systable_beginscan(args_rel, HgEncryptedColumnArgsPkeyIndexId,
								   true, NULL, 1, &args_key);

	while (HeapTupleIsValid(args_tuple = systable_getnext(args_scan)))
	{
		Datum		values[Natts_hg_encrypted_column_args];
		bool		nulls[Natts_hg_encrypted_column_args];
		HeapTuple	new_tuple;
		Oid			new_oid;
		TupleDesc	args_desc = RelationGetDescr(args_rel);
		int			i;

		memset(values, 0, sizeof(values));
		memset(nulls, false, sizeof(nulls));

		new_oid = GetNewOidWithIndex(args_rel, HgEncryptedColumnArgsOidIndexId,
									 Anum_hg_encrypted_column_args_oid);
		values[Anum_hg_encrypted_column_args_oid - 1] = ObjectIdGetDatum(new_oid);
		values[Anum_hg_encrypted_column_args_column_id - 1] = ObjectIdGetDatum(dst_column_id);

		/* function_name / key / value 原样照抄 */
		for (i = Anum_hg_encrypted_column_args_function_name;
			 i <= Anum_hg_encrypted_column_args_value; i++)
		{
			bool		isnull;

			values[i - 1] = heap_getattr(args_tuple, i, args_desc, &isnull);
			nulls[i - 1] = isnull;
		}

		new_tuple = heap_form_tuple(args_desc, values, nulls);
		CatalogTupleInsert(args_rel, new_tuple);

		/* 依赖：args 行 → 新的元数据行（与写入路径一致，保证 DROP 级联清理） */
		{
			ObjectAddress args_addr;
			ObjectAddress enc_addr;

			args_addr.classId = HgEncryptedColumnArgsRelationId;
			args_addr.objectId = new_oid;
			args_addr.objectSubId = 0;

			enc_addr.classId = HgEncryptedColumnsRelationId;
			enc_addr.objectId = dst_column_id;
			enc_addr.objectSubId = 0;

			recordDependencyOn(&args_addr, &enc_addr, DEPENDENCY_AUTO);
		}

		heap_freetuple(new_tuple);
		copied++;
	}

	systable_endscan(args_scan);
	table_close(args_rel, RowExclusiveLock);

	return copied;
}

/*
 * hg_inherit_encrypted_columns
 *		把 parent_oid 上每个加密列的元数据整份复制到 child_oid，返回复制的列数。
 *
 * 用途：CREATE TABLE ... PARTITION OF。分区子表的列定义来自父表，密态元数据
 * 必须与父表逐字段一致。原实现（tablecmds.c 的 PARTITION OF 分支）只抄了
 * column_key_id 与 encryption_type，实测丢两项：
 *   - collation_oid：OPE 字符列的 ICU 排序键，丢了会让子表上的范围/排序语义
 *     偏离父表；
 *   - hg_encrypted_column_args 里的 he_add_dek / he_mul_dek 绑定，丢了会让子表
 *     上的同态运算找不到本列声明的 HE DEK。
 *
 * 这里按整行复制实现：hg_encrypted_columns 行逐字段抄（只换 oid 与 table_oid），
 * 再把父表该列在 hg_encrypted_column_args 的行整行抄过来。不重新解析
 * ENCRYPTED WITH 文本，所以不存在"抄漏某个字段"的问题。
 *
 * 调用时机必须在为分区建索引之前：DefineIndex →
 * check_index_support_for_encrypted_column() 要按 (子表 oid, attnum) 查
 * hg_encrypted_columns，才能拦住"没有 OPE/DET 段却建索引"。
 */
int
hg_inherit_encrypted_columns(Oid parent_oid, Oid child_oid)
{
	Relation	enc_rel;
	SysScanDesc enc_scan;
	ScanKeyData enc_key;
	HeapTuple	parent_tuple;
	int			copied_columns = 0;

	if (!OidIsValid(parent_oid) || !OidIsValid(child_oid))
		return 0;

	enc_rel = table_open(HgEncryptedColumnsRelationId, RowExclusiveLock);

	ScanKeyInit(&enc_key,
				Anum_hg_encrypted_columns_table_oid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(parent_oid));
	enc_scan = systable_beginscan(enc_rel, HgEncryptedColumnsPkeyIndexId,
								  true, NULL, 1, &enc_key);

	while (HeapTupleIsValid(parent_tuple = systable_getnext(enc_scan)))
	{
		Form_hg_encrypted_columns pform;
		Datum		values[Natts_hg_encrypted_columns];
		bool		nulls[Natts_hg_encrypted_columns];
		HeapTuple	child_tuple;
		Oid			child_attnum;
		Oid			child_column_oid;
		Oid			parent_row_oid;
		TupleDesc	enc_desc = RelationGetDescr(enc_rel);

		pform = (Form_hg_encrypted_columns) GETSTRUCT(parent_tuple);
		parent_row_oid = pform->oid;

		/* 按列名对齐：分区子表与父表列名必然相同，attnum 不必相同 */
		child_attnum = get_attnum(child_oid, NameStr(pform->column_name));
		if (child_attnum == InvalidAttrNumber)
			continue;
		if (!is_encrypted_data_type(get_atttype(child_oid, child_attnum)))
			continue;

		memset(values, 0, sizeof(values));
		memset(nulls, false, sizeof(nulls));

		child_column_oid = GetNewOidWithIndex(enc_rel, HgEncryptedColumnsOidIndexId,
											  Anum_hg_encrypted_columns_oid);
		values[Anum_hg_encrypted_columns_oid - 1] = ObjectIdGetDatum(child_column_oid);
		values[Anum_hg_encrypted_columns_table_oid - 1] = ObjectIdGetDatum(child_oid);
		values[Anum_hg_encrypted_columns_column_attnum - 1] =
			Int16GetDatum((int16) child_attnum);
		values[Anum_hg_encrypted_columns_column_name - 1] =
			DirectFunctionCall1(namein, CStringGetDatum(NameStr(pform->column_name)));
		values[Anum_hg_encrypted_columns_data_type_original_oid - 1] =
			ObjectIdGetDatum(pform->data_type_original_oid);
		values[Anum_hg_encrypted_columns_data_type_original_mod - 1] =
			Int32GetDatum(pform->data_type_original_mod);
		values[Anum_hg_encrypted_columns_column_key_id - 1] =
			ObjectIdGetDatum(pform->column_key_id);
		values[Anum_hg_encrypted_columns_encryption_type - 1] =
			Int32GetDatum(pform->encryption_type);
		values[Anum_hg_encrypted_columns_collation_oid - 1] =
			ObjectIdGetDatum(pform->collation_oid);
		values[Anum_hg_encrypted_columns_create_date - 1] =
			TimestampTzGetDatum(GetCurrentTimestamp());

		child_tuple = heap_form_tuple(enc_desc, values, nulls);
		CatalogTupleInsert(enc_rel, child_tuple);

		/* 依赖：元数据行 → 子表列（与 hg_create_encrypted_column 一致） */
		{
			ObjectAddress enc_addr;
			ObjectAddress col_addr;

			enc_addr.classId = HgEncryptedColumnsRelationId;
			enc_addr.objectId = child_column_oid;
			enc_addr.objectSubId = 0;

			col_addr.classId = RelationRelationId;
			col_addr.objectId = child_oid;
			col_addr.objectSubId = child_attnum;

			recordDependencyOn(&enc_addr, &col_addr, DEPENDENCY_AUTO);
		}

		(void) hg_copy_encrypted_column_args(parent_row_oid, child_column_oid);

		heap_freetuple(child_tuple);
		copied_columns++;
	}

	systable_endscan(enc_scan);
	table_close(enc_rel, RowExclusiveLock);

	/*
	 * 让本次插入对紧随其后的 DefineIndex（check_index_support_for_encrypted_column
	 * 走 HGENCRYPTEDCOLUMNSTABLEATTNUM syscache）立即可见。DefineRelation 里
	 * StorePartitionKey 之后本来也会推进一次命令号，这里不引入新的语义。
	 */
	if (copied_columns > 0)
		CommandCounterIncrement();

	return copied_columns;
}

/*
 * 报告分区子表上"有加密类型、父表却没有元数据"的列。
 *
 * 正常路径不会发生（父表建表时已写元数据）；一旦发生，子表这一列永远拿不到
 * 算法/DEK 配置，属于必须让人看见的坏状态，因此保留原有的 WARNING。
 */
static void
hg_warn_uninheritable_encrypted_columns(Oid parent_oid, Oid child_oid)
{
	Relation	child_rel;
	TupleDesc	child_desc;
	int			attidx;

	child_rel = table_open(child_oid, AccessShareLock);
	child_desc = RelationGetDescr(child_rel);

	for (attidx = 0; attidx < child_desc->natts; attidx++)
	{
		Form_pg_attribute att = TupleDescAttr(child_desc, attidx);
		Oid			dek_oid = InvalidOid;
		uint32		enc_type = 0;

		if (att->attisdropped || !is_encrypted_data_type(att->atttypid))
			continue;
		if (hg_get_encrypted_column_info(parent_oid, att->attnum, &dek_oid, &enc_type))
			continue;
		if (hg_get_encrypted_column_info(child_oid, att->attnum, &dek_oid, &enc_type))
			continue;

		ereport(WARNING,
				(errmsg("could not find encrypted column info for inherited column \"%s\" in partition of \"%s\"",
						NameStr(att->attname),
						get_rel_name(parent_oid) ? get_rel_name(parent_oid) : "(unknown)")));
	}

	table_close(child_rel, AccessShareLock);
}

/*
 * hg_process_partition_encrypted_columns
 *		PARTITION OF 专用入口：先把父表的密态列元数据整份继承到子表，再对
 *		无法继承的列发 WARNING。
 *
 *		必须在建分区索引之前调用，原因见 hg_inherit_encrypted_columns()。
 */
void
hg_process_partition_encrypted_columns(Oid parent_oid, Oid child_oid)
{
	if (!OidIsValid(parent_oid) || !OidIsValid(child_oid))
		return;

	(void) hg_inherit_encrypted_columns(parent_oid, child_oid);
	hg_warn_uninheritable_encrypted_columns(parent_oid, child_oid);
}
/* End - ReqID:TECC4.0 - PARTITION OF：密态列元数据整份继承 */

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
	HeapTuple	tuple;
	bool		found = false;

	tuple = SearchSysCache2(HGENCRYPTEDCOLUMNSTABLEATTNUM,
							ObjectIdGetDatum(table_oid),
							Int16GetDatum(attnum));
	if (HeapTupleIsValid(tuple))
	{
		Form_hg_encrypted_columns form;
		form = (Form_hg_encrypted_columns) GETSTRUCT(tuple);
		if (dek_oid)
			*dek_oid = form->column_key_id;
		if (encryption_type_mask)
			*encryption_type_mask = (uint32) form->encryption_type;
		found = true;
		ReleaseSysCache(tuple);
	}

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
								  List *source_tlist, List *rtable,
								  List *parsed_tlist, List *parsed_rtable)
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

				/* ReqID:TECC4.0 - JOIN 查询中规划器将 Var 重写为
				 * INNER_VAR/OUTER_VAR(varno >= 65000)，无法直接定位源表。
				 * 回退到解析后的 target list 查找原始 Var 引用。 */
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

				/* ReqID:TECC4.0 - 回退：规划后 Var 无法定位源表时，
				 * 在解析后的 target list 中查找同类型加密列的原始 Var */
				if (!OidIsValid(src_dek_oid) &&
					(var == NULL || IS_SPECIAL_VARNO(var->varno)) &&
					parsed_tlist != NIL && parsed_rtable != NIL)
				{
					ListCell   *lc_ptle;

					foreach(lc_ptle, parsed_tlist)
					{
						TargetEntry *ptle = (TargetEntry *) lfirst(lc_ptle);
						Var		   *pvar;
						RangeTblEntry *prte;

						if (ptle->resjunk)
							continue;
						if (exprType((Node *) ptle->expr) != typeOid)
							continue;

						if (IsA(ptle->expr, Var))
							pvar = (Var *) ptle->expr;
						else
							pvar = find_first_var_in_expr((Node *) ptle->expr);

						if (pvar == NULL || pvar->varno <= 0 ||
							pvar->varno > list_length(parsed_rtable))
							continue;

						prte = (RangeTblEntry *) list_nth(parsed_rtable,
														  pvar->varno - 1);
						if (prte->rtekind != RTE_RELATION ||
							!OidIsValid(prte->relid))
							continue;

						if (hg_get_encrypted_column_info(prte->relid,
														 pvar->varattno,
														 &src_dek_oid,
														 &src_enc_type))
							break;
					}
				}

				/* ReqID:TECC4.0 - 第二层回退：UNION ALL 等集合操作场景下，
				 * rtable 中无直接 RTE_RELATION 引用（均为子查询）。
				 * 递归进入 RTE_SUBQUERY 的子查询 rtable 查找基表加密列。 */
				if (!OidIsValid(src_dek_oid) &&
					(var == NULL || IS_SPECIAL_VARNO(var->varno)) &&
					parsed_rtable != NIL)
				{
					ListCell   *lc_rte;

					foreach(lc_rte, parsed_rtable)
					{
						RangeTblEntry *prte = (RangeTblEntry *) lfirst(lc_rte);
						ListCell   *lc_srte;

						if (prte->rtekind != RTE_SUBQUERY ||
							prte->subquery == NULL)
							continue;

						/* 进入子查询的 rtable 查找 RTE_RELATION */
						foreach(lc_srte, prte->subquery->rtable)
						{
							RangeTblEntry *srte = (RangeTblEntry *) lfirst(lc_srte);
							int			sattno;

							if (srte->rtekind != RTE_SUBQUERY && srte->rtekind != RTE_RELATION)
								continue;

							for (sattno = 1; sattno <= 100; sattno++)
							{
								if (hg_get_encrypted_column_info(srte->relid,
																 sattno,
																 &src_dek_oid,
																 &src_enc_type))
								{
									if (OidIsValid(src_dek_oid))
										break;
								}
							}
							if (OidIsValid(src_dek_oid))
								break;
						}
						if (OidIsValid(src_dek_oid))
							break;
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
					/* ReqID:TECC4.0 - 继承源列的 collation（与 populate_view_encryption_metadata 一致），
					 * 否则物化视图/CTAS 加密列 collation_locale 为空，
					 * 在非 C locale 环境下 DET/SEARCH 密文不匹配。 */
					hg_params.collation_oid = exprCollation(expr);

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

/*
 * populate_view_encryption_metadata
 *
 * After a VIEW is created, populate hg_encrypted_columns entries for VIEW
 * columns whose types are encrypted data types (int4_enc, text_enc, etc.).
 * Traces each VIEW column back to the source table/column through the view
 * query's target list and range table, then copies the encryption metadata
 * (DEK OID, encryption type) from the source.
 *
 * This enables the TECC client-side cache to recognize VIEW columns as
 * encrypted, allowing automatic decryption when querying through a VIEW.
 */
void
populate_view_encryption_metadata(Oid view_oid, List *targetList,
								  List *rtable)
{
	ListCell   *lc;
	int			attnum;

	if (targetList == NIL || rtable == NIL || !OidIsValid(view_oid))
		return;

	attnum = 0;
	foreach(lc, targetList)
	{
		TargetEntry *tle = (TargetEntry *) lfirst(lc);
		Oid			col_type;
		Var		   *var;
		Oid			src_dek_oid;
		uint32		src_enc_type;

		attnum++;

		/* Skip junk entries (e.g., ORDER BY sort columns) */
		if (tle->resjunk)
			continue;

		col_type = exprType((Node *) tle->expr);

		/* Only process columns with encrypted data types */
		if (!is_encrypted_data_type(col_type))
			continue;

		/* Find the source table reference via the first Var node */
		if (IsA(tle->expr, Var))
			var = (Var *) tle->expr;
		else
			var = find_first_var_in_expr((Node *) tle->expr);

		if (var == NULL || var->varno <= 0 ||
			var->varno > list_length(rtable))
			continue;

		{
			RangeTblEntry *rte;

			rte = (RangeTblEntry *) list_nth(rtable, var->varno - 1);
			if (rte->rtekind != RTE_RELATION || !OidIsValid(rte->relid))
				continue;

			src_dek_oid = InvalidOid;
			src_enc_type = 0;

			if (hg_get_encrypted_column_info(rte->relid, var->varattno,
											 &src_dek_oid, &src_enc_type))
			{
				HgEncryptedColumnCreateParams hg_params;

				memset(&hg_params, 0, sizeof(hg_params));
				hg_params.table_oid = view_oid;
				hg_params.column_attnum = attnum;
				hg_params.column_name = tle->resname;
				hg_params.data_type = col_type;
				hg_params.column_key_id = src_dek_oid;
				hg_params.encryption_type = src_enc_type;
				hg_params.type_mod = exprTypmod((Node *) tle->expr);
				hg_params.collation_oid = exprCollation((Node *) tle->expr);

				hg_create_encrypted_column(&hg_params);
			}
			/* If no source encryption metadata found, silently skip.
			 * This handles cases like SELECT func(col) FROM enc_table
			 * where the expression type differs from the encrypted type. */
		}
	}
}

/* ReqID:TECC4.0 - Encrypted slot validation for ExecConstraints */

/* Begin - ReqID:TECC4.0 - 写入点跨 DEK 赋值防护 */
/*
 * tecc_get_dek_dist_id_by_oid
 *		按 DEK OID 取分布式身份标识（hg_dek.column_key_distributed_id）。
 */
static Oid
tecc_get_dek_dist_id_by_oid(Oid dek_oid)
{
	HeapTuple	tup;
	Oid			dist_id = InvalidOid;

	if (!OidIsValid(dek_oid))
		return InvalidOid;

	tup = SearchSysCache1(HGDEKOID, ObjectIdGetDatum(dek_oid));
	if (HeapTupleIsValid(tup))
	{
		Form_hg_dek	form = (Form_hg_dek) GETSTRUCT(tup);

		dist_id = form->column_key_distributed_id;
		ReleaseSysCache(tup);
	}

	return dist_id;
}

/*
 * tecc_get_column_he_dek_dist_id
 *		取加密列绑定的 HE DEK 分布式身份标识。
 *
 *		value 以十进制文本存入 bytea（见 hg_create_encrypted_column），与密文
 *		段内嵌的 4 字节 DEK ID 是同一体系，故可直接数值比较。
 */
static Oid
tecc_get_column_he_dek_dist_id(Oid encrypted_column_oid, const char *key)
{
	Relation	args_rel;
	SysScanDesc	args_scan;
	ScanKeyData	args_key[2];
	HeapTuple	args_tuple;
	Oid			dist_id = InvalidOid;

	if (!OidIsValid(encrypted_column_oid) || key == NULL)
		return InvalidOid;

	args_rel = table_open(HgEncryptedColumnArgsRelationId, AccessShareLock);

	ScanKeyInit(&args_key[0],
				Anum_hg_encrypted_column_args_column_id,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(encrypted_column_oid));
	ScanKeyInit(&args_key[1],
				Anum_hg_encrypted_column_args_key,
				BTEqualStrategyNumber, F_NAMEEQ,
				CStringGetDatum(key));

	args_scan = systable_beginscan(args_rel, HgEncryptedColumnArgsPkeyIndexId,
								   true, NULL, 2, args_key);

	args_tuple = systable_getnext(args_scan);
	if (HeapTupleIsValid(args_tuple))
	{
		Datum		value;
		bool		isnull;

		value = heap_getattr(args_tuple,
							 Anum_hg_encrypted_column_args_value,
							 RelationGetDescr(args_rel), &isnull);
		if (!isnull)
		{
			char	   *dist_str = TextDatumGetCString(value);

			if (dist_str != NULL && dist_str[0] != '\0')
				dist_id = (Oid) strtoul(dist_str, NULL, 10);
			if (dist_str != NULL)
				pfree(dist_str);
		}
	}

	systable_endscan(args_scan);
	table_close(args_rel, AccessShareLock);

	return dist_id;
}

/*
 * tecc_report_dek_identity_mismatch
 *		报告密文 DEK 身份与目标列配置不一致。
 */
static void
tecc_report_dek_identity_mismatch(const char *colname, const char *segment,
								  Oid expected, Oid actual)
{
	ereport(ERROR,
			(errcode(ERRCODE_TECC_DEK_MISMATCH),
			 errmsg("cannot store ciphertext encrypted with a different DEK into column \"%s\"",
					colname),
			 errdetail("The %s ciphertext carries DEK distributed ID %u, "
					   "but the column is configured with DEK distributed ID %u.",
					   segment, actual, expected),
			 errhint("Encrypt the source value with the DEK the target column uses, "
					 "or assign it to a column that uses the same DEK.")));
}

/*
 * tecc_check_cipher_dek_identity
 *		校验写入某加密列的密文确实由该列配置的 DEK 加密。
 *
 *		密文自带 DEK 身份（CipherDataType.dek_dist_oid 与各 HE 段头部的 4 字节
 *		DEK ID），列上则记录了自己该用哪个 DEK。二者不一致时，密文虽然能按自带
 *		DEK 解出"看似正常"的明文，但该列元数据与实际数据已经背离：
 *		  - 用本列 DEK 加密的常量参与同态运算会报 DEK mismatch；
 *		  - 聚合/比较会静默按密文自带 DEK 计算，跨行结果错误。
 *		因此必须在写入点拒绝，而不是等后续查询出错。
 *
 *		典型触发：UPDATE t1 SET v = (SELECT v FROM t2)、UPDATE t1 SET v = t2.v
 *		FROM t2、以及任何把别的 DEK 的密文写进本列的表达式 —— 这些路径前端都是
 *		原样透传密文，服务端转换函数只校验源密文有 DEK 身份，不比对目标列。
 */
static void
tecc_check_cipher_dek_identity(Form_hg_encrypted_columns form,
							   const char *colname,
							   const void *cipher)
{
	int32		enc_mask = form->encryption_type;
	Oid			sym_id = InvalidOid;
	Oid			he_add_id = InvalidOid;
	Oid			he_mul_id = InvalidOid;
	Oid			expected;
	Oid			actual;

	/* Float sentinel 是 RANGE 分区边界合成值，没有真实 DEK 身份 */
	if (CIPHERDATA_IS_FLOAT_SENTINEL(cipher))
		return;

	if (enc_mask & HG_ENCRYPTION_TYPE_HE_ADD)
		he_add_id = tecc_get_column_he_dek_dist_id(form->oid, "he_add_dek");
	if (enc_mask & HG_ENCRYPTION_TYPE_HE_MUL)
		he_mul_id = tecc_get_column_he_dek_dist_id(form->oid, "he_mul_dek");
	sym_id = tecc_get_dek_dist_id_by_oid(form->column_key_id);

	/*
	 * 主 DEK 身份：有对称 DEK 用对称 DEK，否则回退 HE DEK —— 与前端
	 * tecc_get_column_dek_name 的解析顺序一致（纯 HE 列 dek_dist_oid 就是
	 * 对应 HE DEK 的分布式标识）。
	 */
	expected = OidIsValid(sym_id) ? sym_id :
		(OidIsValid(he_add_id) ? he_add_id : he_mul_id);

	actual = CIPHERDATA_DATUM_DEK_DIST_OID(cipher);

	/*
	 * ReqID:TECC4.0 - 主 DEK 身份允许取本列配置的任一 DEK，不能只认对称 DEK：
	 * 同态运算结果（如 swxa_enc.sql 的 `UPDATE swxa_he_t SET c = a + a`）只含
	 * HE 密文段，其主 DEK 标识记的就是对应 HE DEK；而"带对称 DEK、SUPPORT_ALGO
	 * 仅含 he-add/he-mul"的列正是这类结果的合法目标列（前端放行），若在此按
	 * 对称 DEK 判就会把正常写入打成跨 DEK 错误。
	 */
	if (OidIsValid(expected) && OidIsValid(actual) &&
		actual != expected &&
		!(OidIsValid(he_add_id) && actual == he_add_id) &&
		!(OidIsValid(he_mul_id) && actual == he_mul_id))
		tecc_report_dek_identity_mismatch(colname, "value", expected, actual);

	/* 各 HE 段自带 DEK 身份，混合列（对称 + HE）下与主 DEK 可以不同 */
	if ((enc_mask & HG_ENCRYPTION_TYPE_HE_ADD) &&
		CIPHERDATA_DATUM_EG_ADD_LEN(cipher) > 0)
	{
		actual = CIPHERDATA_DATUM_EG_ADD_DEK_ID(cipher);
		if (OidIsValid(he_add_id) && OidIsValid(actual) && he_add_id != actual)
			tecc_report_dek_identity_mismatch(colname, "he-add", he_add_id, actual);
	}

	if ((enc_mask & HG_ENCRYPTION_TYPE_HE_MUL) &&
		CIPHERDATA_DATUM_EG_MUL_LEN(cipher) > 0)
	{
		actual = CIPHERDATA_DATUM_EG_MUL_DEK_ID(cipher);
		if (OidIsValid(he_mul_id) && OidIsValid(actual) && he_mul_id != actual)
			tecc_report_dek_identity_mismatch(colname, "he-mul", he_mul_id, actual);
	}
}
/* End - ReqID:TECC4.0 - 写入点跨 DEK 赋值防护 */

/* Begin - ReqID:TECC4.0 - 写入点跨算法赋值防护 */
/*
 * tecc_report_missing_algorithm_segment
 *		报告密文缺少目标列声明所需的算法段。
 */
static void
tecc_report_missing_algorithm_segment(const char *colname, const char *segment,
									  const char *declared)
{
	ereport(ERROR,
			(errcode(ERRCODE_TECC_ALGORITHM_INCOMPATIBLE),
			 errmsg("cannot store ciphertext missing the %s segment into column \"%s\"",
					segment, colname),
			 errdetail("The column is declared with support_algo '%s', but the assigned "
					   "value carries no %s ciphertext.",
					   declared, segment),
			 errhint("Assign a value encrypted for this column's algorithms "
					 "(for example from a column with the same support_algo), "
					 "or adjust the column's support_algo.")));
}

/*
 * tecc_check_cipher_segments
 *		校验写入某加密列的密文携带了该列声明所需的全部算法段。
 *
 *		列上记录着本列启用了哪些算法（DET/OPE/SEARCH/he-add/he-mul），密文里则
 *		记录着实际带了哪些段。二者不一致时，列声明的能力落不到数据上：
 *		  - 目标列声明 OPE，却写进只带 DET 的密文：赋值当场成功，读回也"看似
 *		    正常"，直到范围查询才报 "OPE ciphertext not available"；
 *		  - 目标列声明 he-add，却写进对称密文：赋值成功，同态运算报缺段，
 *		    直接 SELECT 还会把原始密文返回给客户端。
 *		因此必须在写入点拒绝，而不是把不可用、不可解的密文留在列里。
 *
 *		正常写入（常量加密、同态运算结果回写）总是携带与列声明一致的段，故本
 *		校验实际只拦"跨算法赋值"。浮点哨兵值（Infinity）没有真实段，跳过。
 *
 *		注：与 tecc_check_cipher_dek_identity 一样，本校验对表内【所有】加密列
 *		都做，不区分该列本次是否被修改 —— 同 DEK 但跨算法的赋值同样必须拦住，
 *		这正是 DEK 身份校验覆盖不到的维度。
 */
static void
tecc_check_cipher_segments(Form_hg_encrypted_columns form,
						   const char *colname,
						   const void *cipher)
{
	int32		enc_mask = form->encryption_type;

	/* Float sentinel 是 RANGE 分区边界合成值，没有真实密文段 */
	if (CIPHERDATA_IS_FLOAT_SENTINEL(cipher))
		return;

	/* Begin - ReqID:TECC4.0 - 对称/搜索类：列声明了就必须带对应段 */
	if (enc_mask & HG_ENCRYPTION_TYPE_DET)
	{
		if (CIPHERDATA_DATUM_DET_LEN(cipher) <= 0)
			tecc_report_missing_algorithm_segment(colname, "DET", "det");
	}
	if (enc_mask & HG_ENCRYPTION_TYPE_OPE)
	{
		if (CIPHERDATA_DATUM_OPE_LEN(cipher) <= 0)
			tecc_report_missing_algorithm_segment(colname, "OPE", "ope");
	}
	if (enc_mask & HG_ENCRYPTION_TYPE_SEARCH)
	{
		if (CIPHERDATA_DATUM_SEARCH_LEN(cipher) <= 0)
			tecc_report_missing_algorithm_segment(colname, "SEARCH", "search");
	}
	/* End - ReqID:TECC4.0 */

	/* Begin - ReqID:TECC4.0 - 同态类：结果密文只有对应 HE 段 */
	if (enc_mask & HG_ENCRYPTION_TYPE_HE_ADD)
	{
		if (CIPHERDATA_DATUM_EG_ADD_LEN(cipher) <= 0)
			tecc_report_missing_algorithm_segment(colname, "he-add", "he-add");
	}
	if (enc_mask & HG_ENCRYPTION_TYPE_HE_MUL)
	{
		if (CIPHERDATA_DATUM_EG_MUL_LEN(cipher) <= 0)
			tecc_report_missing_algorithm_segment(colname, "he-mul", "he-mul");
	}
	/* End - ReqID:TECC4.0 */
}
/* End - ReqID:TECC4.0 - 写入点跨算法赋值防护 */

/* Begin - ReqID:TECC4.0 - 元数据缺失的失败关闭 */
/*
 * tecc_report_missing_column_metadata
 *		某列声明成加密类型、却在 hg_encrypted_columns 里查不到元数据行时调用。
 *
 *		调用点是各消费点"已经确认查不到元数据、正要把它当普通列继续"的位置。
 *		按 hg_tecc.strict_metadata 档位决定：
 *		  error（默认）—— ereport(ERROR)，失败关闭；
 *		  warn          —— ereport(WARNING)，由调用者继续旧行为；
 *		  off           —— 不发任何消息，完全保持旧行为。
 *
 *		column_type 由调用者给出（它手上就有该列的 atttypid）：非加密类型
 *		本来就没有元数据，直接返回。
 *
 *		pg_upgrade / pg_restore 刻意不写元数据（由 hg_tecc.binary_upgrade 标识），
 *		那个窗口内一律不报，否则恢复过程会被自己挡住。
 */
void
tecc_report_missing_column_metadata(Oid table_oid, AttrNumber attnum,
									Oid column_type, const char *consequence)
{
	TeccStrictMetadata level = tecc_get_strict_metadata();
	char	   *colname;
	char	   *relname;

	if (!is_encrypted_data_type(column_type))
		return;
	if (tecc_is_binary_upgrade())
		return;
	if (level == TECC_STRICT_METADATA_OFF)
		return;

	colname = get_attname(table_oid, attnum, true);
	relname = get_rel_name(table_oid);

	if (level == TECC_STRICT_METADATA_WARN)
		ereport(WARNING,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("encryption metadata is missing for column \"%s\" of relation \"%s\"",
						colname ? colname : "?", relname ? relname : "?"),
				 errdetail("The column type is %s, but hg_encrypted_columns has no row for it. %s",
						   format_type_be(column_type), consequence),
				 errhint("Recreate the column with an ENCRYPTED WITH clause. "
						 "Set hg_tecc.strict_metadata to 'error' to fail closed "
						 "instead of continuing.")));
	else
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("encryption metadata is missing for column \"%s\" of relation \"%s\"",
						colname ? colname : "?", relname ? relname : "?"),
				 errdetail("The column type is %s, but hg_encrypted_columns has no row for it. %s",
						   format_type_be(column_type), consequence),
				 errhint("Recreate the column with an ENCRYPTED WITH clause. "
						 "Set hg_tecc.strict_metadata to 'warn' or 'off' to keep "
						 "treating such a column as a plain column.")));
}
/* End - ReqID:TECC4.0 - 元数据缺失的失败关闭 */

/*
 * tecc_validate_encrypted_slots - Validate encrypted column data slots
 *
 * 由 ExecInsert/ExecUpdate/CopyFrom/逻辑复制应用在元组落盘前无条件调用
 * （见 nodeModifyTable.c / execReplication.c / copyfrom.c），对每个加密列：
 *   1. 校验密文自带的 DEK 身份与本列配置的 DEK 一致（跨 DEK 赋值防护）；
 *   2. 校验密文携带了本列 support_algo 声明所需的全部算法段（跨算法赋值防护）。
 *
 * 只有含加密列的表才需要扫 hg_encrypted_columns；本函数先用 tuple
 * descriptor 做一次纯内存判断提前返回，避免给全库普通表的写入路径增加
 * 目录访问开销。
 */
void
tecc_validate_encrypted_slots(ResultRelInfo *resultRelInfo,
							  TupleTableSlot *slot)
{
	Relation	rel = resultRelInfo->ri_RelationDesc;
	TupleDesc	tupdesc = slot->tts_tupleDescriptor;
	Relation	enc_rel;
	SysScanDesc enc_scan;
	ScanKeyData key;
	HeapTuple	enc_tuple;
	Oid			table_oid = RelationGetRelid(rel);
	int			attidx;
	int			enc_attr_count = 0;
	int			meta_row_count = 0;

	/* ReqID:TECC4.0 - 无加密列的表直接返回，不进目录 */
	for (attidx = 0; attidx < tupdesc->natts; attidx++)
	{
		if (is_encrypted_data_type(TupleDescAttr(tupdesc, attidx)->atttypid))
			enc_attr_count++;
	}
	if (enc_attr_count == 0)
		return;

	enc_rel = table_open(HgEncryptedColumnsRelationId, AccessShareLock);

	ScanKeyInit(&key,
				Anum_hg_encrypted_columns_table_oid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(table_oid));

	enc_scan = systable_beginscan(enc_rel,
								  HgEncryptedColumnsPkeyIndexId,
								  true, NULL, 1, &key);

	while (HeapTupleIsValid(enc_tuple = systable_getnext(enc_scan)))
	{
		Form_hg_encrypted_columns form;
		int			attnum;
		Datum		datum;
		bool		is_null;
		Oid			att_type;
		struct varlena *detoasted;

		form = (Form_hg_encrypted_columns) GETSTRUCT(enc_tuple);
		attnum = form->column_attnum;

		if (attnum <= 0 || attnum > tupdesc->natts)
			continue;

		/* Safety: skip if the column type is not an encrypted type */
		att_type = TupleDescAttr(tupdesc, attnum - 1)->atttypid;
		if (!is_encrypted_data_type(att_type))
			continue;

		meta_row_count++;

		datum = slot_getattr(slot, attnum, &is_null);
		if (is_null)
			continue;

		if (!DatumGetPointer(datum))
			continue;

		/*
		 * ReqID:TECC4.0 - Detoast the value before reading the ciphertext
		 * header.  slot_getattr() may return an external (toast) varlena
		 * pointer, whose data area is the toast pointer, not the actual
		 * ciphertext.  The CIPHERDATA_DATUM_* macros read the fixed offset
		 * header via VARDATA_ANY(), which on a toast pointer reads garbage
		 * and spuriously reports an "empty slot" (i8_a constant / i4_x
		 * intermittent).  The HE operators detoast the same way
		 * (he_operators.c).
		 *
		 * 注意：DEK 身份校验与算法段校验对【所有】加密列都要做，不能只盯着
		 * HE 列 —— 对称算法列被写入别的 DEK、或缺少本列算法段的密文同样会
		 * 损坏，且后果更隐蔽（读回"看似正常"，要到比较/范围查询才暴露）。
		 */
		detoasted = PG_DETOAST_DATUM(datum);

		/* ReqID:TECC4.0 - 密文必须由本列配置的 DEK 加密（跨 DEK 赋值防护） */
		tecc_check_cipher_dek_identity(form,
									   NameStr(TupleDescAttr(tupdesc, attnum - 1)->attname),
									   (const void *) detoasted);

		/*
		 * ReqID:TECC4.0 - 密文段必须覆盖本列声明的算法（跨算法赋值防护）。
		 * 原先只对缺失的 HE_ADD/HE_MUL 段发 WARNING，UPDATE 照样成功，列里
		 * 从此留着按本列算法不可用（甚至不可解）的密文；对称/搜索段则完全
		 * 无人校验。现统一在写入点硬报错，见 tecc_check_cipher_segments。
		 */
		tecc_check_cipher_segments(form,
								   NameStr(TupleDescAttr(tupdesc, attnum - 1)->attname),
								   (const void *) detoasted);

		/* Free the detoasted copy if one was allocated */
		if ((Pointer) detoasted != DatumGetPointer(datum))
			pfree(detoasted);
	}

	/*
	 * ReqID:TECC4.0 - 反向检查"有加密类型却没有元数据行"的列。
	 *
	 * 上面的循环是按元数据行驱动的，所以这类列整列被跳过：DEK 身份校验与
	 * 算法段校验都不会发生，别的 DEK 加密的密文、缺段的密文都会被静默写进
	 * 列里，读回"看似正常"，要到比较/范围查询才暴露。
	 *
	 * 只有当元数据行数少于加密列数时才做这次目录查询——健康状态下（两者
	 * 相等）这条路径零额外开销，不给普通表的写入路径加成本。
	 */
	if (meta_row_count < enc_attr_count)
	{
		for (attidx = 0; attidx < tupdesc->natts; attidx++)
		{
			Form_pg_attribute att = TupleDescAttr(tupdesc, attidx);

			if (att->attisdropped || !is_encrypted_data_type(att->atttypid))
				continue;
			if (SearchSysCacheExists2(HGENCRYPTEDCOLUMNSTABLEATTNUM,
									  ObjectIdGetDatum(table_oid),
									  Int16GetDatum(att->attnum)))
				continue;

			tecc_report_missing_column_metadata(table_oid, att->attnum, att->atttypid,
											   "The value written into it is not "
											   "checked for DEK identity or for the "
											   "algorithm segments this column "
											   "declares.");
		}
	}

	systable_endscan(enc_scan);
	table_close(enc_rel, AccessShareLock);
}

/* Begin - ReqID:TECC4.0 - 加密列 ENCRYPTED WITH 子句重建 */
/*
 * hg_encryption_type_to_algo_string
 *		把加密类型位掩码渲染成 SUPPORT_ALGO 的取值。
 *
 *		取值必须保持小写连字符形态（det/ope/search/he-add/he-mul），与
 *		calculate_encryption_type_mask() 接受的 token 逐字对应，这样"子句→
 *		位掩码→子句"是恒等的（大写或 DET/EC-ELGAMAL 那套只用于人读的展示，
 *		见 tecc_tabledef.c 的 append_encryption_type_algo）。
 */
static void
hg_encryption_type_to_algo_string(uint32 encryption_type, StringInfo buf)
{
	uint32		masks[] = {HG_ENCRYPTION_TYPE_DET, HG_ENCRYPTION_TYPE_OPE,
	HG_ENCRYPTION_TYPE_SEARCH, HG_ENCRYPTION_TYPE_HE_ADD,
	HG_ENCRYPTION_TYPE_HE_MUL};
	const char *names[] = {"det", "ope", "search", "he-add", "he-mul"};
	bool		first = true;
	int			i;

	for (i = 0; i < lengthof(masks); i++)
	{
		if ((encryption_type & masks[i]) == 0)
			continue;
		if (!first)
			appendStringInfoChar(buf, ',');
		appendStringInfoString(buf, names[i]);
		first = false;
	}
}

/*
 * hg_dek_qualified_name
 *		取 DEK 的 "模式名.密钥名"（palloc 结果）。
 *
 *		带模式名是为了让重建出来的定义不依赖接收端的 search_path ——
 *		Citus 分片 DDL 是在 worker 的会话里按 worker 自己的 search_path
 *		解析 DEK 名字的。
 */
static char *
hg_dek_qualified_name(Oid dek_oid)
{
	HeapTuple	tup;
	Form_hg_dek	form;
	char	   *nspname;
	char	   *result;

	tup = SearchSysCache1(HGDEKOID, ObjectIdGetDatum(dek_oid));
	if (!HeapTupleIsValid(tup))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("DEK with OID %u does not exist", dek_oid)));

	form = (Form_hg_dek) GETSTRUCT(tup);
	nspname = get_namespace_name(form->key_namespace);
	result = psprintf("%s.%s", nspname != NULL ? nspname : "",
					  NameStr(form->dek_name));

	ReleaseSysCache(tup);
	if (nspname != NULL)
		pfree(nspname);

	return result;
}

/*
 * hg_dek_qualified_name_by_dist_id
 *		按 DEK 分布式身份标识取 "模式名.密钥名"；找不到返回 NULL。
 *
 *		同态 DEK 绑定（hg_encrypted_column_args）里存的是分布式身份标识而不是
 *		hg_dek.oid，故此处按 column_key_distributed_id 反查。
 */
static char *
hg_dek_qualified_name_by_dist_id(Oid dist_id)
{
	Relation	rel;
	SysScanDesc	scan;
	ScanKeyData	key[1];
	HeapTuple	tup;
	char	   *result = NULL;

	if (!OidIsValid(dist_id))
		return NULL;

	rel = table_open(HgDekRelationId, AccessShareLock);
	ScanKeyInit(&key[0],
				Anum_hg_dek_column_key_distributed_id,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(dist_id));
	scan = systable_beginscan(rel, HgDekDistributedIdIndexId, true, NULL, 1, key);

	tup = systable_getnext(scan);
	if (HeapTupleIsValid(tup))
	{
		Form_hg_dek	form = (Form_hg_dek) GETSTRUCT(tup);
		char	   *nspname = get_namespace_name(form->key_namespace);

		result = psprintf("%s.%s", nspname != NULL ? nspname : "",
						  NameStr(form->dek_name));
		if (nspname != NULL)
			pfree(nspname);
	}

	systable_endscan(scan);
	table_close(rel, AccessShareLock);

	return result;
}

/*
 * hg_get_encrypted_column_clause
 *		重建一个加密列的 ENCRYPTED WITH 子句文本（含前导空格）。
 *
 *		返回 palloc 的字符串，形如
 *			" ENCRYPTED WITH (DEK = 'public.d1', SUPPORT_ALGO = 'det,ope')"
 *		该列没有加密元数据时返回 NULL。
 *
 *		为什么需要它：ENCRYPTED WITH 子句只落在 hg_encrypted_columns 里，
 *		pg_attribute 不保存它。任何"只按元组描述符重建表定义"的路径都会把子句
 *		丢掉，接收端见到裸的 *_enc 类型就直接报
 *		`encrypted data type "int4_enc" requires ENCRYPTED WITH clause`
 *		（Citus 建分片表正是此路径：分片 DDL 由 TupleDesc 反解析而成，worker
 *		再用本 fork 的语法重新解析执行）。
 *
 *		选项键与取值形态必须与 src/bin/pg_dump/pg_dump.c 的渲染一致
 *		（DEK / SUPPORT_ALGO / HE_ADD_DEK / HE_MUL_DEK），否则要么解析不过，
 *		要么静默丢掉列上的算法声明与同态密钥绑定。
 *
 *		*has_ope 非 NULL 时回填"该列算法是否含 OPE"：只有 OPE 的排序依赖排序
 *		规则，调用方若同时要输出 COLLATE，其余情况必须抑制，否则会被
 *		parse_expr.c 的加密列 COLLATE 守卫拒掉。
 */
char *
hg_get_encrypted_column_clause(Oid table_oid, AttrNumber attnum, bool *has_ope)
{
	HeapTuple	tuple;
	Form_hg_encrypted_columns form;
	StringInfoData buf;
	uint32		encryption_type;
	uint32		he_masks[2] = {HG_ENCRYPTION_TYPE_HE_ADD, HG_ENCRYPTION_TYPE_HE_MUL};
	const char *he_arg_keys[2] = {"he_add_dek", "he_mul_dek"};
	const char *he_opt_names[2] = {"HE_ADD_DEK", "HE_MUL_DEK"};
	bool		first_option = true;
	char	   *dek_name;
	int			i;

	if (has_ope != NULL)
		*has_ope = false;

	tuple = SearchSysCache2(HGENCRYPTEDCOLUMNSTABLEATTNUM,
							ObjectIdGetDatum(table_oid),
							Int16GetDatum(attnum));
	if (!HeapTupleIsValid(tuple))
		return NULL;

	form = (Form_hg_encrypted_columns) GETSTRUCT(tuple);
	encryption_type = (uint32) form->encryption_type;

	initStringInfo(&buf);
	appendStringInfoString(&buf, " ENCRYPTED WITH (");

	/*
	 * 对称 DEK。纯同态列没有对称 DEK（column_key_id 无效），此时必须不输出
	 * DEK 选项 —— 否则接收端会拿空名字去 get_dek_oid() 而报错。
	 */
	if (OidIsValid(form->column_key_id))
	{
		dek_name = hg_dek_qualified_name(form->column_key_id);
		appendStringInfo(&buf, "DEK = %s", quote_literal_cstr(dek_name));
		first_option = false;
	}

	if (encryption_type != 0)
	{
		appendStringInfoString(&buf, first_option ? "" : ", ");
		appendStringInfoString(&buf, "SUPPORT_ALGO = '");
		hg_encryption_type_to_algo_string(encryption_type, &buf);
		appendStringInfoChar(&buf, '\'');
		first_option = false;
	}

	/*
	 * he-add / he-mul 各自的同态 DEK 绑定（一列多 DEK，P0-2）。位掩码里有该
	 * 算法位但 args 行缺失时只能跳过：宁可少一个绑定，也不要让整条分片 DDL
	 * 生成失败（缺失本身由 W2b 的失败关闭开关负责暴露）。
	 */
	for (i = 0; i < lengthof(he_masks); i++)
	{
		Oid			dist_id;

		if ((encryption_type & he_masks[i]) == 0)
			continue;

		dist_id = tecc_get_column_he_dek_dist_id(form->oid, he_arg_keys[i]);
		dek_name = hg_dek_qualified_name_by_dist_id(dist_id);
		if (dek_name == NULL)
			continue;

		appendStringInfo(&buf, "%s%s = %s", first_option ? "" : ", ",
						 he_opt_names[i], quote_literal_cstr(dek_name));
		first_option = false;
	}

	appendStringInfoChar(&buf, ')');

	if (has_ope != NULL)
		*has_ope = ((encryption_type & HG_ENCRYPTION_TYPE_OPE) != 0);

	ReleaseSysCache(tuple);

	return buf.data;
}

/*
 * hg_get_encrypted_column_options
 *		把加密列的 ENCRYPTED WITH 选项重建为**解析树**形态（DefElem 列表），
 *		即 gram.y 写进 ColumnDef.encrypted_with 的那种结构。
 *
 *		为什么要重建解析树：分片 DDL 到达 worker 时只有表名与列类型，
 *		ENCRYPTED WITH 子句在反解析阶段就已经丢了（子句只存在
 *		hg_encrypted_columns 里）。DefineRelation()/ATExecAddColumn() 判的是
 *		解析树里的 encrypted_with，不认目录内容，所以必须在
 *		ProcessUtility_hook 里把子句**放回解析树**才能让分片表落地。
 *		渲染成文本（hg_get_encrypted_column_clause）只够喂给反解析器，喂不了
 *		建表流程。两条路径的取值必须逐字一致，改动时两边一起改。
 *
 *		该列没有加密元数据时返回 NIL。
 */
List *
hg_get_encrypted_column_options(Oid table_oid, AttrNumber attnum)
{
	HeapTuple	tuple;
	Form_hg_encrypted_columns form;
	List	   *options = NIL;
	uint32		encryption_type;
	uint32		he_masks[2] = {HG_ENCRYPTION_TYPE_HE_ADD, HG_ENCRYPTION_TYPE_HE_MUL};
	char	   *he_arg_keys[2] = {"he_add_dek", "he_mul_dek"};
	char	   *dek_name;
	int			i;

	tuple = SearchSysCache2(HGENCRYPTEDCOLUMNSTABLEATTNUM,
							ObjectIdGetDatum(table_oid),
							Int16GetDatum(attnum));
	if (!HeapTupleIsValid(tuple))
		return NIL;

	form = (Form_hg_encrypted_columns) GETSTRUCT(tuple);
	encryption_type = (uint32) form->encryption_type;

	/*
	 * 对称 DEK。纯同态列没有对称 DEK（column_key_id 无效），此时必须不输出
	 * DEK 选项 —— 与文本重建路径保持一致。
	 */
	if (OidIsValid(form->column_key_id))
	{
		dek_name = hg_dek_qualified_name(form->column_key_id);
		options = lappend(options,
						  makeDefElem("dek", (Node *) makeString(dek_name), -1));
	}

	if (encryption_type != 0)
	{
		StringInfoData buf;

		initStringInfo(&buf);
		hg_encryption_type_to_algo_string(encryption_type, &buf);
		options = lappend(options,
						  makeDefElem("support_algo",
									  (Node *) makeString(buf.data), -1));
	}

	/*
	 * he-add / he-mul 各自的同态 DEK 绑定。位掩码里有该算法位但 args 行缺失时
	 * 只能跳过（与文本重建路径同口径）：宁可少一个绑定，也不要让整条 DDL
	 * 生成失败，缺失本身由 W2b 的失败关闭开关负责暴露。
	 */
	for (i = 0; i < lengthof(he_masks); i++)
	{
		Oid			dist_id;

		if ((encryption_type & he_masks[i]) == 0)
			continue;

		dist_id = tecc_get_column_he_dek_dist_id(form->oid, he_arg_keys[i]);
		dek_name = hg_dek_qualified_name_by_dist_id(dist_id);
		if (dek_name == NULL)
			continue;

		options = lappend(options,
						  makeDefElem(he_arg_keys[i],
									  (Node *) makeString(dek_name), -1));
	}

	ReleaseSysCache(tuple);

	return options;
}
/* End - ReqID:TECC4.0 - 加密列 ENCRYPTED WITH 子句重建 */

/* Begin - ReqID:TECC4.0 - 供 libpq DDL-only 通路同进程查询（弱符号） */
/*
 * hg_get_encrypted_clause_by_name
 *		按（模式名, 表名, 列名）返回该列 ENCRYPTED WITH 子句文本（含前导空格）。
 *
 *		谁在用：libpq 的 `TECC_MODE_DDL_ONLY`（分布式内部连接）。插件下发的 worker
 *		侧 DDL 是按元组描述符反解析出来的、不带子句，而下发它的进程**就是**协调节点
 *		后端，本进程目录里正好有这份声明 —— 包括两种情况：
 *		  - 本事务尚未提交的行（例如 ALTER ADD COLUMN 刚写的那一行）；
 *		  - 目标节点上什么都没有的时候（加节点建壳表：新节点的 pg_dist_node 还是空的，
 *		    跨会话取数连"协调节点在哪"都问不出来）。
 *
 *		返回值用 malloc 分配（调用方在 libpq 里 free()），取不到返回 NULL。
 *
 *		**绝不 ereport**：调用方在 libpq 的调用栈上，longjmp 会把 libpq 的连接状态
 *		留在半截。整段包在 PG_TRY 里，任何错误一律降级成"取不到"（调用方据此不补
 *		子句 → worker 侧照旧报 requires ENCRYPTED WITH clause，失败关闭）。
 */
char *
hg_get_encrypted_clause_by_name(const char *schema, const char *table,
								const char *column)
{
	char	   *result = NULL;
	MemoryContext oldcxt;

	if (schema == NULL || table == NULL || column == NULL)
		return NULL;
	if (table[0] == '\0' || column[0] == '\0')
		return NULL;

	oldcxt = CurrentMemoryContext;

	PG_TRY();
	{
		Oid			relid = InvalidOid;

		/*
		 * 不带模式名是正常形态（插件下发的 ALTER TABLE 就是 `ALTER TABLE t
		 * ADD COLUMN c ...`）：按本进程的 search_path 解析即可 —— 协调节点就是
		 * 建表的那一端，解析结果与目标节点后续执行时一致。
		 */
		if (schema[0] == '\0')
			relid = RelnameGetRelid(table);
		else
		{
			Oid			nsp = get_namespace_oid(schema, true);

			if (OidIsValid(nsp))
				relid = get_relname_relid(table, nsp);
		}

		if (OidIsValid(relid))
		{
			AttrNumber	attnum = get_attnum(relid, column);

			if (attnum != InvalidAttrNumber)
			{
				char	   *clause = hg_get_encrypted_column_clause(relid, attnum,
																	NULL);

				if (clause != NULL)
				{
					result = strdup(clause);
					pfree(clause);
				}
			}
		}
	}
	PG_CATCH();
	{
		MemoryContextSwitchTo(oldcxt);
		FlushErrorState();
		result = NULL;
	}
	PG_END_TRY();

	return result;
}
/* End - ReqID:TECC4.0 - 供 libpq DDL-only 通路同进程查询（弱符号） */
