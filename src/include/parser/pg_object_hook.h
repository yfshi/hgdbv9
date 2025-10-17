/*--------------------------------------------------------------------
 * 
 * File: pg_object_hook.h
 *
 * Abstract: 
 * 		This file contains hooks declarations for pg_object.
 *
 * Authored by qinshiyu@highgo.com, 20240410.
 *
 * Copyright:
 * Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		src/include/parser/pg_object_hook.h
 *
 * add the file for requirement "SRS-SQL-Object"
 *
 *--------------------------------------------------------------------
 */

#ifndef PG_OBJECT_HOOK_H
#define PG_OBJECT_HOOK_H

#include "postgres.h"

#include "c.h"
#include "access/htup.h"
#include "nodes/pg_list.h"
#include "nodes/parsenodes.h"
#include "catalog/objectaddress.h"
#include "parser/parse_node.h"
#include "utils/jsonb.h"

#define  PG_OBJECT_RELATION 'r'				/* relation */
#define  PG_OBJECT_VIEW 'v'					/* view */
#define  PG_OBJECT_MATVIEW 'm'				/* matview */
#define  PG_OBJECT_PARTITION_RELATION 'P'	/* partition relation */
#define  PG_OBJECT_FOREIGN_RELATION 'F'		/* foreign relation */
#define  PG_OBJECT_PROCEDURE 'p'			/* procedure */
#define  PG_OBJECT_WINDOW 'w'				/* window */
#define  PG_OBJECT_FUNCTION 'f'				/* function */
#define  PG_OBJECT_AGGREGATE 'a'			/* aggregate */
#define  PG_OBJECT_SCHEMA 's'				/* schema */
#define  PG_OBJECT_DATABASE 'd'				/* database */
#define  PG_OBJECT_TRIGGER 't'				/* trigger */
#define  PG_OBJECT_COLUMN 'c'				/* column, used by zerofill */
#define  PG_OBJECT_DBLINK 'l'				/* compatible Oracle DBLINK */
#define  PG_OBJECT_RESGROUP 'g'				/* resource group of role */ /* ReqID:SRS-SECURITY-RESOURCEMANAGEMENT */
#define  PG_OBJECT_ENUM 'e'					/* anonymous_enum */ /* ReqID:SRS-MYSQL-ENUMTYPE */

typedef struct pg_object_ori_infor
{
	char		*oriname;		/* The object name stored in pg_object */
	Oid			orischema;		/* The object schema stored in pg_object */
	char 		orikind;		/* The object kind */
	char		oricompatible_mode;	/* The object compatible_mode */
	Timestamp	*orimodifytime;	/* The object midify time stored in pg_object */
	Jsonb		*orioption;		/* The object option stored in pg_object */
}pg_object_ori_infor;
typedef pg_object_ori_infor *PgObjectInfo;

typedef void (*pg_object_create_obj_hook_type) (char *objname, Oid objnsp, bool replace, char objkind,
															ObjectAddress address, Jsonb *attr, Timestamp *createtime);
typedef void (*pg_object_alter_obj_hook_type) (ObjectAddress address, char *newname, Oid newschema, Jsonb *attr, Timestamp *modifiedtime);
typedef PgObjectInfo (*pg_object_get_ori_infor_hook_type) (ObjectAddress address);
typedef void (*pg_object_drop_obj_hook_type) (ObjectAddress address);
typedef void (*pg_object_bulid_cache_obj_hook_type) (Relation relation);



extern PGDLLIMPORT pg_object_create_obj_hook_type pg_object_create_obj_hook;
extern PGDLLIMPORT pg_object_alter_obj_hook_type pg_object_alter_obj_hook;
extern PGDLLIMPORT pg_object_drop_obj_hook_type pg_object_drop_obj_hook;
extern PGDLLIMPORT pg_object_get_ori_infor_hook_type pg_object_get_ori_infor_hook;
extern PGDLLIMPORT pg_object_bulid_cache_obj_hook_type pg_object_bulid_cache_obj_hook;
extern PGDLLIMPORT pg_object_bulid_cache_obj_hook_type pg_object_bulid_relcache;

/* Begin - Req:SRS-SQL-NLSSORT */
extern Oid  GetNamespaceCollation(Oid namespaceId);
extern void SetNamespaceCollation(char *namespaceName, Oid namespaceId, Oid coll, ObjectAddress address);
extern Oid  GetRelationCollation(Relation relation);
extern void SetRelationCollation(char *relname, Oid namespaceId, Oid coll, ObjectAddress address);
extern void UpdateRelationCollation(char *relname, Oid namespaceId, Oid coll, ObjectAddress address);
/* End - Req:SRS-SQL-NLSSORT */

/* Begin - ReqID:RSR-SQL-Partition-interval */
extern char *GetRelationInterval(Oid relid);
extern void SetRelationInterval(Oid relid, char *relname, Oid namespace, char relkind, char *value);
extern void DeleteRelationInterval(Oid relid);
/* End - ReqID:RSR-SQL-Partition-interval */

#endif
