/*--------------------------------------------------------------------------
 *
 * test_rls_hooks.h
 *		Definitions for RLS hooks
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *		src/mysql_test/modules/test_rls_hooks/test_rls_hooks.h
 *
 * 	 	add the file for requirement "SRS-MISC-MYSQL-FRAME"
 *
 * -------------------------------------------------------------------------
 */

#ifndef TEST_RLS_HOOKS_H
#define TEST_RLS_HOOKS_H

#include <rewrite/rowsecurity.h>

/* Return set of permissive hooks based on CmdType and Relation */
extern List *test_rls_hooks_permissive(CmdType cmdtype, Relation relation);

/* Return set of restrictive hooks based on CmdType and Relation */
extern List *test_rls_hooks_restrictive(CmdType cmdtype, Relation relation);

#endif
