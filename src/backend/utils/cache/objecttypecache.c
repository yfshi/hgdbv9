/*-------------------------------------------------------------------------
 *
 * objecttypecache.c
 *	  Oracle object type cache support.
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/htup_details.h"
#include "access/table.h"
#include "catalog/indexing.h"
#include "catalog/pg_object_type.h"
#include "catalog/pg_object_type_body.h"
#include "funcapi.h"
#include "utils/catcache.h"
#include "utils/guc.h"
#include "utils/hsearch.h"
#include "utils/inval.h"
#include "utils/memutils.h"
#include "utils/objecttypecache.h"
#include "utils/plancache.h"
#include "utils/syscache.h"

static HTAB *ObjectTypeCache = NULL;
static MemoryContext ObjectTypeCacheContext = NULL;

static void BuildObjectTypeCache(void);
static void InvalidateObjectTypeCacheCallback(Datum arg, int cacheid,
											  uint32 hashvalue);

/*
 * ObjectTypeCacheLookup
 *    Look up an object type cache item by its OID.
 *    Initializes the cache on first call if needed.
 */
ObjectTypeCacheItem *
ObjectTypeCacheLookup(ObjectTypeCacheKey *key)
{
	ObjectTypeCacheEntry *entry;

	if (ObjectTypeCache == NULL)
		BuildObjectTypeCache();

	entry = hash_search(ObjectTypeCache, key, HASH_FIND, NULL);
	return entry == NULL ? NULL : entry->item;
}

/*
 * ObjectTypeCacheIsValid
 *    Check if a cached object type is still valid.
 *    Validates both the specification and optionally the body.
 *    Uses either tuple-based or status-based invalidation depending on GUC.
 */
bool
ObjectTypeCacheIsValid(ObjectTypeCacheKey *key, bool check_body)
{
	ObjectTypeCacheItem *item;
	HeapTuple	tuple;
	bool		is_valid = true;

	Assert(key != NULL);

	/* Check if the object type exists in system catalog */
	tuple = SearchSysCache1(OBJTYPEOID, ObjectIdGetDatum(*key));
	if (!HeapTupleIsValid(tuple))
		return false;

	/* Check if cache entry exists and is valid */
	item = ObjectTypeCacheLookup(key);
	if (item == NULL)
		is_valid = false;
	else if (check_package_invalid_by_tuple)
	{
		/* Compare tuple version info for invalidation */
		if (item->object_xmin != HeapTupleHeaderGetRawXmin(tuple->t_data) ||
			!ItemPointerEquals(&item->object_tid, &tuple->t_self))
			is_valid = false;
	}
	else if (OBJECT_TYPE_SPECIFICATION_IS_UPDATED(item->cachestatus))
		is_valid = false;

	ReleaseSysCache(tuple);
	if (!is_valid)
		return false;

	/* Optionally validate the body */
	if (check_body)
	{
		if (check_package_invalid_by_tuple)
		{
			tuple = SearchSysCache1(OBJTYPEBODYOBJTYPOID, ObjectIdGetDatum(*key));
			if (!HeapTupleIsValid(tuple))
				return false;

			if (item->body_xmin != HeapTupleHeaderGetRawXmin(tuple->t_data) ||
				!ItemPointerEquals(&item->body_tid, &tuple->t_self))
				is_valid = false;
			ReleaseSysCache(tuple);
		}
		else if (!OBJECT_TYPE_HAS_BODY(item->cachestatus) ||
				 OBJECT_TYPE_BODY_IS_UPDATED(item->cachestatus))
			is_valid = false;
	}

	return is_valid;
}

/*
 * ObjectTypeHandle
 *    Get or compile an object type, returning its cache item.
 *    Delegates to the PL/iSQL handler for compilation if not cached.
 */
ObjectTypeCacheItem *
ObjectTypeHandle(Oid objectid, bool is_body)
{
	plisql_internel_funcs_init();

	if (plisql_internal_funcs.object_type_handle == NULL)
		elog(ERROR, "object type cache handler is not registered");

	return (ObjectTypeCacheItem *)
		plisql_internal_funcs.object_type_handle(objectid, is_body);
}

void
ObjectTypeValidate(Oid objectid, bool is_body)
{
	plisql_internel_funcs_init();

	if (plisql_internal_funcs.object_type_validator == NULL)
		elog(ERROR, "object type validator is not registered");

	plisql_internal_funcs.object_type_validator(objectid, is_body);
}

/*
 * ObjectTypeHandleByTypid
 *    Get or compile an object type by its underlying type OID.
 *    Used when we have the pg_type OID and need the object type cache.
 */
ObjectTypeCacheItem *
ObjectTypeHandleByTypid(Oid typoid, bool missing_ok)
{
	Oid			objoid;

	/* Look up object type OID from the base type OID */
	objoid = GetSysCacheOid1(OBJTYPETYPOID, Anum_pg_object_type_oid,
							 ObjectIdGetDatum(typoid));
	if (!OidIsValid(objoid))
	{
		if (missing_ok)
			return NULL;
		elog(ERROR, "object type for base type %u does not exist", typoid);
	}

	/* Return cached version if valid, otherwise compile */
	if (ObjectTypeCacheIsValid(&objoid, true))
		return ObjectTypeCacheLookup(&objoid);

	return ObjectTypeHandle(objoid, false);
}

/*
 * ObjectTypeCacheDelete
 *    Remove an object type from the cache.
 *    Also invalidates plan cache entries that depend on this object type.
 */
ObjectTypeCacheItem *
ObjectTypeCacheDelete(ObjectTypeCacheKey *key)
{
	ObjectTypeCacheEntry *entry;

	if (ObjectTypeCache == NULL)
		return NULL;

	entry = (ObjectTypeCacheEntry *) hash_search(ObjectTypeCache, key,
												 HASH_REMOVE, NULL);
	if (entry != NULL)
	{
		setPlanCacheInvalidForObjectType(*key);
		return entry->item;
	}

	return NULL;
}

/*
 * ObjectTypeCacheInsert
 *    Insert a new object type into the cache.
 *    Errors if an entry with the same key already exists.
 */
void
ObjectTypeCacheInsert(ObjectTypeCacheKey *key, ObjectTypeCacheItem *item)
{
	ObjectTypeCacheEntry *entry;
	bool		found;

	if (ObjectTypeCache == NULL)
		BuildObjectTypeCache();

	entry = (ObjectTypeCacheEntry *) hash_search(ObjectTypeCache, key,
												 HASH_ENTER, &found);
	if (found)
		elog(ERROR, "trying to insert an object type cache item that already exists");

	entry->key = *key;
	entry->item = item;
	item->intable = true;
}

/*
 * set_objectinfo_from_funcexpr
 *    Populate function expression with object type info for method calls.
 *    Looks up or compiles the object type and sets parent_func pointer.
 */
void
set_objectinfo_from_funcexpr(FuncExpr *expr)
{
	ObjectTypeCacheItem *item;

	Assert(expr->function_from == FUNC_FROM_OBJECT);

	/* Already resolved */
	if (expr->parent_func != NULL)
		return;

	if (!OidIsValid(expr->pkgoid))
		elog(ERROR, "object method expression has invalid object oid");

	/* Get object type from cache or compile it */
	if (ObjectTypeCacheIsValid(&expr->pkgoid, true))
		item = ObjectTypeCacheLookup(&expr->pkgoid);
	else
		item = ObjectTypeHandle(expr->pkgoid, false);

	if (item == NULL || item->source == NULL)
		elog(ERROR, "object type %u cache is not found", expr->pkgoid);

	expr->parent_func = item->source;
}

/*
 * InvalidObjectType
 *    Invalidate an object type cache entry.
 *    Either updates the tuple directly or marks cache status as updated.
 */
void
InvalidObjectType(Oid objoid)
{
	HeapTuple	tup;
	Relation	rel;
	ObjectTypeCacheItem *item = NULL;

	if (check_package_invalid_by_tuple)
		rel = table_open(ObjectTypeRelationId, RowExclusiveLock);
	else
		rel = table_open(ObjectTypeRelationId, AccessShareLock);

	tup = SearchSysCacheCopy1(OBJTYPEOID, ObjectIdGetDatum(objoid));
	if (!HeapTupleIsValid(tup))
	{
		table_close(rel, NoLock);
		return;
	}

	if (check_package_invalid_by_tuple)
		CatalogTupleUpdate(rel, &tup->t_self, tup);
	else
	{
		/* Mark cache entry as having updated specification */
		item = ObjectTypeCacheLookup(&objoid);
		CacheInvalidateHeapTuple(rel, tup, NULL);
		if (item != NULL)
			item->cachestatus =
				OBJECT_TYPE_SET_SPECIFICATION_UPDATE_FLAG(item->cachestatus);
	}

	heap_freetuple(tup);
	table_close(rel, NoLock);
}

/*
 * InvalidObjectTypeBody
 *    Invalidate an object type body cache entry.
 *    Marks the parent object type's body as updated in cache.
 */
void
InvalidObjectTypeBody(Oid bodyoid)
{
	HeapTuple	tup;
	Relation	rel;
	ObjectTypeCacheItem *item = NULL;
	Oid			objoid = InvalidOid;

	if (check_package_invalid_by_tuple)
		rel = table_open(ObjectTypeBodyRelationId, RowExclusiveLock);
	else
		rel = table_open(ObjectTypeBodyRelationId, AccessShareLock);

	tup = SearchSysCacheCopy1(OBJTYPEBODYOID, ObjectIdGetDatum(bodyoid));
	if (!HeapTupleIsValid(tup))
	{
		table_close(rel, NoLock);
		return;
	}

	objoid = ((Form_pg_object_type_body) GETSTRUCT(tup))->objtypoid;

	if (check_package_invalid_by_tuple)
		CatalogTupleUpdate(rel, &tup->t_self, tup);
	else
	{
		/* Mark cache entry as having updated body */
		item = ObjectTypeCacheLookup(&objoid);
		CacheInvalidateHeapTuple(rel, tup, NULL);
		if (item != NULL)
			item->cachestatus =
				OBJECT_TYPE_SET_BODY_UPDATE_FLAG(item->cachestatus);
	}

	heap_freetuple(tup);
	table_close(rel, NoLock);
}

/*
 * BuildObjectTypeCache
 *    Initialize the object type cache hash table and memory context.
 *    Registers cache invalidation callbacks for object type system catalogs.
 */
static void
BuildObjectTypeCache(void)
{
	HASHCTL		ctl;
	MemoryContext oldcontext;

	Assert(ObjectTypeCacheContext == NULL);

	if (CacheMemoryContext == NULL)
		CreateCacheMemoryContext();

	ObjectTypeCacheContext =
		AllocSetContextCreate(CacheMemoryContext, "ObjectTypeCache",
							  ALLOCSET_DEFAULT_SIZES);

	/* Register invalidation callbacks when not using tuple-based checking */
	if (!check_package_invalid_by_tuple)
	{
		CacheRegisterSyscacheCallback(OBJTYPEOID,
									  InvalidateObjectTypeCacheCallback,
									  (Datum) 0);
		CacheRegisterSyscacheCallback(OBJTYPEBODYOID,
									  InvalidateObjectTypeCacheCallback,
									  (Datum) 0);
	}

	oldcontext = MemoryContextSwitchTo(ObjectTypeCacheContext);
	MemSet(&ctl, 0, sizeof(ctl));
	ctl.keysize = sizeof(ObjectTypeCacheKey);
	ctl.entrysize = sizeof(ObjectTypeCacheEntry);
	ctl.hcxt = ObjectTypeCacheContext;
	ObjectTypeCache = hash_create("Object Type Cache", 32, &ctl,
								  HASH_ELEM | HASH_BLOBS | HASH_CONTEXT);
	MemoryContextSwitchTo(oldcontext);
}

/*
 * InvalidateObjectTypeCacheCallback
 *    Cache invalidation callback for object type system catalog changes.
 *    Marks affected cache entries as updated when catalog tuples change.
 */
static void
InvalidateObjectTypeCacheCallback(Datum arg, int cacheid, uint32 hashvalue)
{
	HASH_SEQ_STATUS status;
	ObjectTypeCacheEntry *entry;

	if (ObjectTypeCache == NULL)
		return;

	hash_seq_init(&status, ObjectTypeCache);
	while ((entry = (ObjectTypeCacheEntry *) hash_seq_search(&status)) != NULL)
	{
		bool		invalid = false;

		/* Check if this entry matches the invalidation event */
		if (hashvalue == 0)
			invalid = true;
		else if (cacheid == OBJTYPEOID &&
				 entry->item->object_hash_value == hashvalue)
			invalid = true;
		else if (cacheid == OBJTYPEBODYOID &&
				 entry->item->body_hash_value == hashvalue)
			invalid = true;

		if (!invalid)
			continue;

		/* Mark the appropriate part of the cache entry as updated */
		if (cacheid == OBJTYPEOID)
			entry->item->cachestatus =
				OBJECT_TYPE_SET_SPECIFICATION_UPDATE_FLAG(entry->item->cachestatus);
		else
			entry->item->cachestatus =
				OBJECT_TYPE_SET_BODY_UPDATE_FLAG(entry->item->cachestatus);
	}
}
