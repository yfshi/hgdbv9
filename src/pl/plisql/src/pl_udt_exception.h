/*-------------------------------------------------------------------------
 *
 * File:pl_udt_exception.h
 *
 * Abstract:
 *		Definitions for the PLISQL
 *			  procedural language user define exception type
 *
 * ReqID:SRS-PLSQL-EXCEPTION
 *
 * Authored by dwdai@highgo.com,20240806.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  src/pl/plisql/src/pl_udt_exception.h
 *
 *-------------------------------------------------------------------------
 */

#ifndef PLISQL_UDT_EXCEPTION_H
#define PLISQL_UDT_EXCEPTION_H

#include "plisql.h"
#include "utils/palloc.h"

#define PLISQL_UDT_EXCEPTION_DEFAULT_ERRMSG "User-Defined Exception"
#define PLISQL_UDT_EXCEPTION_DEFAULT_SQLCODE 1


/* must match contrib/ivorysql_ora/src/ora_exception/ora_exception--1.0.sql */
#define ORA_EXCEPTION_MAP_TABLE_NAME "_ivorysql_ora_exception_map"
#define ORA_EXCEPTION_MAP_ORACODE_INDEX_NAME "_ivorysql_ora_exception_map_oracode"
#define ORA_EXCEPTION_MAP_PGCODE_INDEX_NAME "_ivorysql_ora_exception_map_pgsqlcode"


enum
{
	ORA_EXCEPTION_MAP_BY_PGCODE,
	ORA_EXCEPTION_MAP_BY_ORACODE
};

enum Anum_ivorysql_ora_exception_map_column_idx
{
	Anum_ivorysql_ora_exceptionmap_pg_sqlcode_idx = 1,
	Anum_ivorysql_ora_exceptionmap_ora_errcode_idx,
	Anum_ivorysql_ora_exceptionmap_message_idx
};

extern void init_plisql_error_stack(void);
extern int get_current_sqlcode_sqlerrm(char **psqlerrm);
extern void push_error_stack(int newcode, char *sqlerrm);
extern void pop_error_stack(void);
extern bool set_current_error_stack(int newcode, char *newerrm,
									int *oldcode, char **olderrm,
									bool shoud_pop,
									bool should_push);
extern void pop_error_stack_bydeep(int curr_dep);
extern int get_current_error_stack_deep(void);
extern char *format_error_stack_msg(void);
extern int plisql_get_exception_oraerrcode(MemoryContext stmt_txt,
													int pgerrcode,
													char *origin_msg,
													char **errmsg);
extern int plisql_get_exception_pgerrcode(MemoryContext stmt_txt,
													int oraerrcode,
													char **errmsg);
extern void plisql_set_udt_exception_errcode(PLiSQL_var *exception_var,
													int oraerrcode);
#endif   /* PLISQL_UDT_EXCEPTION_H */

