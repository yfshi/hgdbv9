/*---------------------------------------------------------------------------------
*
* File:dbms_utility.h
*
* Abstract:
*		Internal implementation of DBMS_UTILITY package
*
* ReqID: SRS-PLSQL-DBMS_UTILITY
* Copyright:
* Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
*
* IDENTIFICATION
*	  contrib/ivorysql_ora/src/builtin_packages/dbms_utility/dbms_utility.h
*
*---------------------------------------------------------------------------------
*/
#ifndef DBMS_UTILITY_H
#define DBMS_UTILITY_H

#include "postgres.h"
#include "funcapi.h"
#include "storage/proc.h"
#include "utils/builtins.h"
#include "tcop/tcopprot.h"
#include "nodes/parsenodes.h"
#include "parser/analyze.h"
#include "catalog/pg_class.h"
#include "nodes/nodes.h"
#include "utils/ruleutils.h"
#include "fmgr.h"
#include "utils/elog.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#include <bits/time.h>
#endif

void collect_error_message(ErrorData* edata);
void collect_error_message_start(void);
void collect_error_message_finish(void);
void collect_error_message_inc_depth(void);
void collect_error_message_dec_depth(void);

#endif      /* DBMS_UTILITY_H */
