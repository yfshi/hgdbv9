/*-------------------------------------------------------------------------
 *
 * objecttypecache.h
 *	  Oracle object type cache support.
 *
 *-------------------------------------------------------------------------
 */

#ifndef OBJECTTYPECACHE_H
#define OBJECTTYPECACHE_H

#include "c.h"
#include "nodes/primnodes.h"
#include "storage/itemptr.h"

/* Cache status flags for object type invalidation tracking */
typedef char ObjectTypeCacheStatus;

/* Object type specification has been updated (needs recompile) */
#define OBJECT_TYPE_SPECIFICATION_UPDATE_FLAG 0x01
/* Object type has a body defined */
#define OBJECT_TYPE_HAS_BODY_FLAG			 0x02
/* Object type body has been updated (needs recompile) */
#define OBJECT_TYPE_BODY_UPDATE_FLAG		 0x04

/* Check if specification has been updated */
#define OBJECT_TYPE_SPECIFICATION_IS_UPDATED(_status) \
	(_status & OBJECT_TYPE_SPECIFICATION_UPDATE_FLAG)
/* Check if body has been updated */
#define OBJECT_TYPE_BODY_IS_UPDATED(_status) \
	(_status & OBJECT_TYPE_BODY_UPDATE_FLAG)
/* Check if object type has a body */
#define OBJECT_TYPE_HAS_BODY(_status) \
	(_status & OBJECT_TYPE_HAS_BODY_FLAG)

/* Mark specification as updated */
#define OBJECT_TYPE_SET_SPECIFICATION_UPDATE_FLAG(_status) \
	(_status | OBJECT_TYPE_SPECIFICATION_UPDATE_FLAG)
/* Mark as having a body */
#define OBJECT_TYPE_SET_BODY_FLAG(_status) \
	(_status | OBJECT_TYPE_HAS_BODY_FLAG)
/* Mark body as updated */
#define OBJECT_TYPE_SET_BODY_UPDATE_FLAG(_status) \
	(_status | OBJECT_TYPE_BODY_UPDATE_FLAG)

/* Cache key is the object type OID */
typedef Oid ObjectTypeCacheKey;

/*
 * ObjectTypeCacheItem
 *    In-memory cache entry for a compiled object type.
 *    Stores version info for invalidation and the compiled source.
 */
typedef struct ObjectTypeCacheItem
{
	ObjectTypeCacheKey key;			/* Object type OID */
	TransactionId object_xmin;		/* Xmin of pg_object_type tuple */
	ItemPointerData object_tid;		/* TID of pg_object_type tuple */
	TransactionId body_xmin;		/* Xmin of pg_object_type_body tuple */
	ItemPointerData body_tid;		/* TID of pg_object_type_body tuple */
	MemoryContext ctxt;				/* Memory context for this cache item */
	uint32		object_hash_value;	/* Hash value for object type syscache */
	uint32		body_hash_value;	/* Hash value for body syscache */
	bool		intable;			/* True if entry is in hash table */
	ObjectTypeCacheStatus cachestatus; /* Invalidation status flags */
	void	   *source;				/* Compiled PL/iSQL source (PLiSQL_function) */
} ObjectTypeCacheItem;

/*
 * ObjectTypeCacheEntry
 *    Hash table entry for the object type cache.
 */
typedef struct ObjectTypeCacheEntry
{
	ObjectTypeCacheKey key;
	ObjectTypeCacheItem *item;
} ObjectTypeCacheEntry;

extern ObjectTypeCacheItem *ObjectTypeCacheLookup(ObjectTypeCacheKey *key);
extern bool ObjectTypeCacheIsValid(ObjectTypeCacheKey *key, bool check_body);
extern ObjectTypeCacheItem *ObjectTypeHandle(Oid objectid, bool is_body);
extern ObjectTypeCacheItem *ObjectTypeHandleByTypid(Oid typoid, bool missing_ok);
extern ObjectTypeCacheItem *ObjectTypeCacheDelete(ObjectTypeCacheKey *key);
extern void ObjectTypeCacheInsert(ObjectTypeCacheKey *key, ObjectTypeCacheItem *item);
extern void ObjectTypeValidate(Oid objectid, bool is_body);
extern void InvalidObjectType(Oid objoid);
extern void InvalidObjectTypeBody(Oid bodyoid);
extern void set_objectinfo_from_funcexpr(FuncExpr *expr);

#endif
