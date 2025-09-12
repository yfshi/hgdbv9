/*-------------------------------------------------------------------------
 *
 * mysset.h
 *	  Header file for the implementation of the MySQL SET feature hook
 *
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 *
 * IDENTIFICATION
 *	  contrib/ivorysql_my/src/include/mysset.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef MYSSET_H
#define MYSSET_H

#include "postgres.h"
#include "catalog/pg_type.h"
#include "nodes/pathnodes.h"
#include "parser/parse_node.h"
#include "tcop/cmdtag.h"
#include "utils/builtins.h"
#include "utils/hsearch.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"

#define ERROR_CHARSET(CHARSET) \
	(pg_strcasecmp(CHARSET, "ucs2") == 0 || \
	 pg_strcasecmp(CHARSET, "utf16") == 0 || \
	 pg_strcasecmp(CHARSET, "utf16le") == 0 || \
	 pg_strcasecmp(CHARSET, "utf32") == 0 \
	)

extern Node *userVariable_Hash_Search(ParseState *pstate,
								 VarRef *varRef);
extern void mysSetUserVarInternal(char *userVarName,
								  Oid consttype,
								  int32 consttypmod,
								  Oid constcollid,
								  int constlen,
								  Datum constvalue,
								  bool constisnull,
								  bool constbyval);
#endif
