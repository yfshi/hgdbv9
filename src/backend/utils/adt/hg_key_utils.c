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