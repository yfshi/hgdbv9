/*-------------------------------------------------------------------------
 *
 * mys_set_hook.h
 *	  Define MySQL SET hook
 *
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 *
 * IDENTIFICATION
 *	  src/include/mysql_parser/mys_set_hook.h
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "fmgr.h"
#include "miscadmin.h"
#include "catalog/pg_type.h"
#include "nodes/pathnodes.h"
#include "parser/parse_node.h"
#include "tcop/cmdtag.h"
#include "utils/builtins.h"
#include "utils/hsearch.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"

typedef Node *(* uservariable_hash_search_hook_type)(ParseState *pstate,
									VarRef *varref);
extern PGDLLIMPORT uservariable_hash_search_hook_type uservariable_hash_search_hook;
typedef void (* SetUserVarInternal_hook_type)(char *userVarName,
									   Oid consttype,
									   int32 consttypmod,
									   Oid constcollid,
									   int constlen,
									   Datum constvalue,
									   bool constisnull,
									   bool constbyval);
extern PGDLLIMPORT SetUserVarInternal_hook_type setuservarinternal_hook;