/*-------------------------------------------------------------------------
 *
 * hg_key_utils.c
 *	  Utility functions for HG key management
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/backend/utils/adt/hg_key_utils.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/heapam.h"
#include "access/htup.h"
#include "access/htup_details.h"
#include "access/sysattr.h"
#include "access/genam.h"

#include "catalog/hg_kek.h"
#include "catalog/hg_dek.h"
#include "catalog/hg_dek_args.h"
#include "catalog/hg_encrypted_columns.h"
#include "catalog/hg_encrypted_column_args.h"
#include "catalog/indexing.h"
#include "catalog/namespace.h"
#include "catalog/pg_authid.h"
#include "utils/acl.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/hg_key_utils.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"
#include "utils/snapmgr.h"
#include "utils/syscache.h"

/*
 * get_kek_oid_ext - given a KEK name and namespace, look up the OID
 *
 * If namespace_id is InvalidOid, searches all namespaces.
 */
Oid
get_kek_oid_ext(const char *kekname, Oid namespace_id, bool missing_ok)
{
	Oid			oid = InvalidOid;
	Relation	rel;
	TableScanDesc scan;
	HeapTuple	tup;

	rel = table_open(HgKekRelationId, AccessShareLock);

	if (OidIsValid(namespace_id))
	{
		ScanKeyData key[2];

		ScanKeyInit(&key[0],
					Anum_hg_kek_kek_name,
					BTEqualStrategyNumber, F_NAMEEQ,
					CStringGetDatum(kekname));
		ScanKeyInit(&key[1],
					Anum_hg_kek_key_namespace,
					BTEqualStrategyNumber, F_OIDEQ,
					ObjectIdGetDatum(namespace_id));

		scan = table_beginscan(rel, GetActiveSnapshot(), 2, key);
		tup = heap_getnext(scan, ForwardScanDirection);
		if (HeapTupleIsValid(tup))
		{
			Form_hg_kek form = (Form_hg_kek) GETSTRUCT(tup);
			oid = form->oid;
		}
	}
	else
	{
		/* Search all namespaces, return first match */
		ScanKeyData key[1];

		ScanKeyInit(&key[0],
					Anum_hg_kek_kek_name,
					BTEqualStrategyNumber, F_NAMEEQ,
					CStringGetDatum(kekname));

		scan = table_beginscan(rel, GetActiveSnapshot(), 1, key);
		tup = heap_getnext(scan, ForwardScanDirection);
		if (HeapTupleIsValid(tup))
		{
			Form_hg_kek form = (Form_hg_kek) GETSTRUCT(tup);
			oid = form->oid;
		}
	}

	table_endscan(scan);
	table_close(rel, AccessShareLock);

	if (!OidIsValid(oid) && !missing_ok)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("KEK \"%s\" does not exist", kekname)));
	return oid;
}

/*
 * get_kek_oid - given a KEK name, look up the OID via search_path
 *
 * If the name contains a dot (schema-qualified, e.g. "public.my_kek"),
 * resolves the schema explicitly via LookupExplicitNamespace and looks up
 * only in that namespace.  Otherwise searches through all namespaces in
 * the current search_path (including implicit schemas like pg_catalog).
 */
Oid
get_kek_oid(const char *kekname, bool missing_ok)
{
	List	   *search_path;
	ListCell   *lc;
	Oid			oid;

	/* ReqID:TECC4.0 - Support schema-qualified KEK names (schema.kek_name) */
	{
		const char *dot;

		dot = strchr(kekname, '.');
		if (dot != NULL && dot != kekname && dot[1] != '\0')
		{
			char		schema_name[NAMEDATALEN];
			size_t		schema_len;
			Oid			namespace_id;

			schema_len = dot - kekname;
			if (schema_len >= NAMEDATALEN)
				schema_len = NAMEDATALEN - 1;
			memcpy(schema_name, kekname, schema_len);
			schema_name[schema_len] = '\0';

			namespace_id = LookupExplicitNamespace(schema_name, missing_ok);
			if (!OidIsValid(namespace_id))
				return InvalidOid;

			return get_kek_oid_ext(dot + 1, namespace_id, missing_ok);
		}
	}

	/* Get full search path (including implicit schemas) */
	search_path = fetch_search_path(true);

	foreach(lc, search_path)
	{
		Oid nspid = lfirst_oid(lc);

		oid = get_kek_oid_ext(kekname, nspid, true);
		if (OidIsValid(oid))
			return oid;
	}

	if (!missing_ok)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("KEK \"%s\" does not exist", kekname)));
	return InvalidOid;
}

/*
 * get_dek_oid - given a DEK name, look up the OID via search_path
 *
 * If the name contains a dot (schema-qualified, e.g. "public.my_dek"),
 * resolves the schema explicitly via LookupExplicitNamespace and looks up
 * only in that namespace.  Otherwise searches through all namespaces in
 * the current search_path (including implicit schemas like pg_catalog).
 */
Oid
get_dek_oid(const char *dekname, bool missing_ok)
{
	List	   *search_path;
	ListCell   *lc;
	Oid			oid;

	/* ReqID:TECC4.0 - Support schema-qualified DEK names (schema.dek_name) */
	{
		const char *dot;

		dot = strchr(dekname, '.');
		if (dot != NULL && dot != dekname && dot[1] != '\0')
		{
			char		schema_name[NAMEDATALEN];
			size_t		schema_len;
			Oid			namespace_id;

			schema_len = dot - dekname;
			if (schema_len >= NAMEDATALEN)
				schema_len = NAMEDATALEN - 1;
			memcpy(schema_name, dekname, schema_len);
			schema_name[schema_len] = '\0';

			namespace_id = LookupExplicitNamespace(schema_name, missing_ok);
			if (!OidIsValid(namespace_id))
				return InvalidOid;

			return get_dek_oid_ext(dot + 1, namespace_id, missing_ok);
		}
	}

	/* Get full search path (including implicit schemas) */
	search_path = fetch_search_path(true);

	foreach(lc, search_path)
	{
		Oid nspid = lfirst_oid(lc);

		oid = get_dek_oid_ext(dekname, nspid, true);
		if (OidIsValid(oid))
			return oid;
	}

	if (!missing_ok)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("DEK \"%s\" does not exist", dekname)));
	return InvalidOid;
}

/*
 * get_dek_oid_ext - given a DEK name and namespace, look up the OID
 */
Oid
get_dek_oid_ext(const char *dekname, Oid namespace_id, bool missing_ok)
{
	Oid			oid;
	Relation	rel;
	TableScanDesc scan;
	HeapTuple	tup;
	ScanKeyData key[2];

	/* Open hg_dek relation */
	rel = table_open(HgDekRelationId, AccessShareLock);

	/* Setup scan keys for dek_name and key_namespace */
	ScanKeyInit(&key[0],
				Anum_hg_dek_dek_name,
				BTEqualStrategyNumber, F_NAMEEQ,
				CStringGetDatum(dekname));
	ScanKeyInit(&key[1],
				Anum_hg_dek_key_namespace,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(namespace_id));

	/* Start heap scan */
	scan = table_beginscan(rel, GetActiveSnapshot(), 2, key);

	tup = heap_getnext(scan, ForwardScanDirection);
	if (HeapTupleIsValid(tup))
	{
		Form_hg_dek form = (Form_hg_dek) GETSTRUCT(tup);
		oid = form->oid;
	}
	else
	{
		oid = InvalidOid;
	}

	table_endscan(scan);
	table_close(rel, AccessShareLock);

	if (!OidIsValid(oid) && !missing_ok)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("DEK \"%s\" does not exist in namespace %u",
						dekname, namespace_id)));
	return oid;
}

/*
 * get_kek_name - given a KEK OID, look up the name
 */
char *
get_kek_name(Oid kekid)
{
	HeapTuple	tup;
	Form_hg_kek form;
	char	   *result;

	tup = SearchSysCache1(HGKEKOID, ObjectIdGetDatum(kekid));
	if (!HeapTupleIsValid(tup))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("KEK with OID %u does not exist", kekid)));

	form = (Form_hg_kek) GETSTRUCT(tup);
	result = pstrdup(NameStr(form->kek_name));

	ReleaseSysCache(tup);

	return result;
}

/*
 * get_dek_name - given a DEK OID, look up the name
 */
char *
get_dek_name(Oid dekid)
{
	HeapTuple	tup;
	Form_hg_dek form;
	char	   *result;

	tup = SearchSysCache1(HGDEKOID, ObjectIdGetDatum(dekid));
	if (!HeapTupleIsValid(tup))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("DEK with OID %u does not exist", dekid)));

	form = (Form_hg_dek) GETSTRUCT(tup);
	result = pstrdup(NameStr(form->dek_name));

	ReleaseSysCache(tup);

	return result;
}

/*
 * kek_exists - check if a KEK exists
 */
bool
kek_exists(Oid kekid)
{
	return SearchSysCacheExists1(HGKEKOID, ObjectIdGetDatum(kekid));
}

/*
 * dek_exists - check if a DEK exists
 */
bool
dek_exists(Oid dekid)
{
	return SearchSysCacheExists1(HGDEKOID, ObjectIdGetDatum(dekid));
}

/*
 * Note: has_kek_usage_privilege() and has_dek_usage_privilege() functions have been removed.
 * Use has_kek_privilege() and has_dek_privilege() functions from acl.c instead.
 */


/*
 * is_valid_dek - validate DEK
 *
 * In TECC 4.0 design, DEK doesn't have a status field.
 * A DEK is considered valid if:
 * 1. It exists in the hg_dek table
 * 2. Its associated KEK exists (kek_id is valid)
 */
bool
is_valid_dek(Oid dekid)
{
	HeapTuple	tup;
	bool		is_valid = false;
	bool		isnull;
	Datum		kek_id_datum;

	tup = SearchSysCache1(HGDEKOID, ObjectIdGetDatum(dekid));
	if (!HeapTupleIsValid(tup))
		return false;

	/* Get kek_id datum */
	kek_id_datum = heap_getattr(tup, Anum_hg_dek_kek_id,
								RelationGetDescr(RelationIdGetRelation(HgDekRelationId)),
								&isnull);

	/* Check if DEK has a valid KEK */
	if (!isnull && kek_exists(DatumGetObjectId(kek_id_datum)))
		is_valid = true;

	ReleaseSysCache(tup);

	return is_valid;
}

/* Begin - ReqID:TECC4.0 */
/*
 * get_dek_arg_value - retrieve the value of a named hg_dek_args entry
 *
 * Given a DEK OID and argument key name (e.g. "key_algo"),
 * returns the text value stored in hg_dek_args, or NULL if not found.
 * The caller is responsible for pfree()ing the result.
 *
 * This is used by the server-side HE operator code to determine
 * the HE key type (EC-ElGamal vs finite field ElGamal).
 */
char *
get_dek_arg_value(Oid dek_id, const char *arg_key)
{
	Relation	rel;
	SysScanDesc scan;
	ScanKeyData key[2];
	HeapTuple	tup;
	char	   *result = NULL;

	rel = table_open(HgDekArgsRelationId, AccessShareLock);

	ScanKeyInit(&key[0],
				Anum_hg_dek_args_dek_id,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(dek_id));
	ScanKeyInit(&key[1],
				Anum_hg_dek_args_key,
				BTEqualStrategyNumber, F_NAMEEQ,
				CStringGetDatum(arg_key));

	scan = systable_beginscan(rel, HgDekArgsPkeyIndexId, true,
							  NULL, 2, key);

	tup = systable_getnext(scan);
	if (HeapTupleIsValid(tup))
	{
		Form_hg_dek_args form = (Form_hg_dek_args) GETSTRUCT(tup);
		Datum		val_datum;
		bool		isnull;
		bytea	   *ba;

		val_datum = heap_getattr(tup, Anum_hg_dek_args_value,
								 RelationGetDescr(rel), &isnull);
		if (!isnull)
		{
			ba = DatumGetByteaPP(val_datum);
			result = (char *) palloc(VARSIZE_ANY_EXHDR(ba) + 1);
			memcpy(result, VARDATA_ANY(ba), VARSIZE_ANY_EXHDR(ba));
			result[VARSIZE_ANY_EXHDR(ba)] = '\0';
		}
	}

	systable_endscan(scan);
	table_close(rel, AccessShareLock);

	return result;
}
/* End - ReqID:TECC4.0 */

/* Begin - ReqID:TECC4.0 */
/*
 * get_dek_dist_id - 按 DEK OID 取分布式身份标识
 *
 *		即 hg_dek.column_key_distributed_id：由"模式名.密钥名"哈希得出的跨实例
 *		确定性 DEK 身份。密文段内嵌的 4 字节 DEK ID 与加密列上的同态 DEK 绑定
 *		存的都是这个值，因此它既用于解密时反查 DEK，也用于校验"密文是不是由本列
 *		配置的 DEK 加密的"。
 */
Oid
get_dek_dist_id(Oid dek_id)
{
	HeapTuple	tup;
	Oid			dist_id = InvalidOid;

	if (!OidIsValid(dek_id))
		return InvalidOid;

	tup = SearchSysCache1(HGDEKOID, ObjectIdGetDatum(dek_id));
	if (HeapTupleIsValid(tup))
	{
		dist_id = ((Form_hg_dek) GETSTRUCT(tup))->column_key_distributed_id;
		ReleaseSysCache(tup);
	}

	return dist_id;
}

/*
 * find_he_dek_bound_column - 查找把该 DEK 当作同态密钥绑定的加密列
 *
 *		同态 DEK 不落在 hg_encrypted_columns.column_key_id 上，而是作为
 *		'he_add_dek' / 'he_mul_dek' 条目写进 hg_encrypted_column_args，value
 *		存 DEK 分布式身份标识的十进制文本。所以只按 column_key_id 判断"密钥
 *		是否仍被列占用"的检查漏得掉纯同态列：列还引用着密钥，密钥却能删掉；
 *		而重建同名 DEK 得到的分布式身份标识与原来完全相同，写入点那条按该标识
 *		做的跨 DEK 校验会照样通过，遗留密文却已解不开。
 *
 *		出参均可传 NULL（只作存在性判断时）。arg_key 指向静态字面量，调用方
 *		无需释放；column_name 为 palloc 结果，调用方负责 pfree。
 */
bool
find_he_dek_bound_column(Oid dek_dist_id, Oid *table_oid,
						 char **column_name, const char **arg_key)
{
	Relation	args_rel;
	SysScanDesc	args_scan;
	HeapTuple	args_tuple;
	char		needle[16];
	int			needle_len;
	bool		found = false;

	if (!OidIsValid(dek_dist_id))
		return false;

	needle_len = snprintf(needle, sizeof(needle), "%u", (unsigned int) dek_dist_id);

	args_rel = table_open(HgEncryptedColumnArgsRelationId, AccessShareLock);
	args_scan = systable_beginscan(args_rel, InvalidOid, false, NULL, 0, NULL);

	while ((args_tuple = systable_getnext(args_scan)) != NULL)
	{
		Form_hg_encrypted_column_args args_form;
		const char *key;
		Datum		value;
		bool		isnull;
		bytea	   *bound_id;

		args_form = (Form_hg_encrypted_column_args) GETSTRUCT(args_tuple);
		key = NameStr(args_form->key);
		if (strcmp(key, "he_add_dek") != 0 && strcmp(key, "he_mul_dek") != 0)
			continue;

		value = heap_getattr(args_tuple, Anum_hg_encrypted_column_args_value,
							 RelationGetDescr(args_rel), &isnull);
		if (isnull)
			continue;

		bound_id = DatumGetByteaPP(value);
		if ((int) VARSIZE_ANY_EXHDR(bound_id) != needle_len ||
			memcmp(VARDATA_ANY(bound_id), needle, needle_len) != 0)
			continue;

		if (table_oid != NULL || column_name != NULL)
		{
			Relation	enc_rel;
			ScanKeyData enc_key[1];
			SysScanDesc	enc_scan;
			HeapTuple	enc_tuple;
			Oid			tbl_oid = InvalidOid;
			char	   *col_name = NULL;

			enc_rel = table_open(HgEncryptedColumnsRelationId, AccessShareLock);
			ScanKeyInit(&enc_key[0],
						Anum_hg_encrypted_columns_oid,
						BTEqualStrategyNumber, F_OIDEQ,
						ObjectIdGetDatum(args_form->column_id));
			enc_scan = systable_beginscan(enc_rel, HgEncryptedColumnsOidIndexId,
										  true, NULL, 1, enc_key);
			enc_tuple = systable_getnext(enc_scan);
			if (HeapTupleIsValid(enc_tuple))
			{
				Form_hg_encrypted_columns enc_form =
					(Form_hg_encrypted_columns) GETSTRUCT(enc_tuple);

				tbl_oid = enc_form->table_oid;
				col_name = pstrdup(NameStr(enc_form->column_name));
			}
			systable_endscan(enc_scan);
			table_close(enc_rel, AccessShareLock);

			if (table_oid != NULL)
				*table_oid = tbl_oid;
			if (column_name != NULL)
				*column_name = col_name;
			else if (col_name != NULL)
				pfree(col_name);
		}

		if (arg_key != NULL)
			*arg_key = (strcmp(key, "he_add_dek") == 0) ?
				"he_add_dek" : "he_mul_dek";

		found = true;
		break;
	}

	systable_endscan(args_scan);
	table_close(args_rel, AccessShareLock);

	return found;
}
/* End - ReqID:TECC4.0 */
