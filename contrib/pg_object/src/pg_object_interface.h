/*--------------------------------------------------------------------
 *
 * File: casesensitive.h
 *
 * Abstract:
 * 		This file contains extern declarations for ivorysql_my itself.
 *
 * Authored by qinshiyu@highgo.com, 20240410.
 *
 * Copyright:
 * Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *		contrib/ivorysql_my/src/include/casesensitive.h
 *
 * add the file for requirement "SRS-SQL-Object"
 *
 *--------------------------------------------------------------------
 */
#ifndef PG_OBJECT_INTERFACE_H_
#define PG_OBJECT_INTERFACE_H_

#include "postgres.h"
#include "access/htup.h"
#include "nodes/pg_list.h"
#include "nodes/parsenodes.h"
#include "catalog/objectaddress.h"
#include "parser/parse_node.h"
#include "parser/pg_object_hook.h"
#include "utils/jsonb.h"

typedef enum RelAttrFillOption
{
	OBJECT_FILL_NOTHING,
	OBJECT_FILL_REL,
	OBJECT_FILL_ATTR
}RelAttrFillOption;

extern bool enable_pg_object;

extern void pg_object_createobj(char *objname, Oid objnsp, bool replace, char objkind,
										  ObjectAddress address, Jsonb *attr, Timestamp *createtime);
extern void pg_object_alterobj(ObjectAddress address, char *newname, Oid newschema, Jsonb *attr, Timestamp *modifiedtime);
extern PgObjectInfo get_object_origin_info(ObjectAddress address);
extern void pg_object_drop(ObjectAddress address);
extern void RelationBuildColumnAttr(Relation relation);

#endif
