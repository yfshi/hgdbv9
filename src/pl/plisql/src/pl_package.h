/*-------------------------------------------------------------------------
 *
 * File:pl_package.h
 *
 * Abstract:
 *		Definitions for the PLISQL
 *			  procedural language package
 *
 * ReqID:SRS-SQL-PACKAGE
 *
 * Authored by dwdai@highgo.com,20221017.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  src/pl/plisql/src/pl_package.h
 *
 *-------------------------------------------------------------------------
 */

#ifndef PLSQL_PACKAGE_H
#define PLSQL_PACKAGE_H
#include "plisql.h"
#include "pl_subproc_function.h"
#include "parser/parse_package.h"

typedef PLiSQL_function package_function;

/*
 * package status
 * if package specificiation has only constants and types,
 * then package has no status others package has status.
 * if package has status, first we reference package'variable or function
 * then we set the package status to PLSQL_PACKAGE_INIT if it has
 * status and when we rebuild package, we set the package'status to
 * PLiSQL_PACKAGE_REBUILD
*/
typedef enum
{
	PLISQL_PACKAGE_NO_STATUS = 0,		 /* package has no status */
	PLISQL_PACKAGE_NO_STATUS_INIT,		 /* package has no status but init */
	PLISQL_PACKAGE_NO_INIT, 			 /* package has status but hasn't init */
	PLISQL_PACKAGE_INIT,				 /* pakcage has status and already init */
	PLISQL_PACKAGE_STATUSED,			 /* package has status and accesed by its var
										  * and function
										  */
	PLISQL_PACKAGE_REBUILD,				 /* package has been rebuilt, but not init */
	PLISQL_PACKAGE_REBUILD_INIT			 /* package has been rebuild and  init */
} PLiSQL_package_status;


/*
 * special_cur: only include package specification
 * last_globaldno: the last package global dno
 * laist_specialfno: then last package special fno
 */

typedef struct PLiSQL_package
{
	PLiSQL_nsitem *special_cur; 	 /* namespace search for package */
	int	 last_globaldno;		 /* package'globaldno */
	int	last_specialfno;		/* package special last fno */
	PLiSQL_package_status status;	 /* package status */
	package_function source;		 /* package and its body source */
	bool	 finish_compile_special; /* package special has been compile */
} PLiSQL_package;


typedef enum
{
	parse_by_var_type = 0, 		 /* parse by pkg.var%type */
	parse_by_var_rowtype,			 /* parse by pkg.var%rowtype */
	parse_by_pkg_type				 /* parse by pkg.type */
} parse_pkgtype_flag;


/*
 * in during compile function, we may be compile a package
 * so we must save compile global var, this struct include
 * all compile global var
 */
typedef struct PLiSQL_compile_global_proper
{
	/* plisql.h */
	IdentifierLookup plisql_IdentifierLookup;
	int	plisql_variable_conflict;
	bool plisql_print_strict_params;
	bool plisql_check_asserts;
	int	plisql_extra_warnings;
	int	plisql_extra_errors;
	bool plisql_check_syntax;
	bool plisql_DumpExecTree;
	PLiSQL_stmt_block *plisql_parse_result;
	int	plisql_nDatums;
	PLiSQL_datum **plisql_Datums;
	int datums_last;
	char *plisql_error_funcname;
	PLiSQL_function *plisql_curr_compile;
	MemoryContext plisql_compile_tmp_cxt;
	/* Begin - ReqID:SRS-PLSQL-TYPE */
	bool check_referenced_objects;
	List *plisql_referenced_objects;
	/* End - ReqID:SRS-PLSQL-TYPE */
	/* Begin - BUG#M0000201: support question marks as binding variables */
	int questionmarks_count;
	/* End - BUG#M0000201: support question marks as binding variables */

	/* pl_subproc_function.h */
	PLiSQL_subproc_function **plisql_subprocFuncs;
	int plisql_nsubprocFuncs;
	int subprocFuncs_alloc;
	int cur_compile_func_level;
	PLiSQL_function *plisql_saved_compile[FUNC_MAX_NEST_LEVEL];
	int plisql_saved_datums_last[FUNC_MAX_NEST_LEVEL];
	char *plisql_saved_error_funcname[FUNC_MAX_NEST_LEVEL];
	int plisql_saved_nsubprocfuncs[FUNC_MAX_NEST_LEVEL];
	int plisql_saved_subprocfunc_alloc[FUNC_MAX_NEST_LEVEL];
	PLiSQL_subproc_function **plisql_saved_subprocfuncs[FUNC_MAX_NEST_LEVEL];

	/* pl_package.h */
	PLiSQL_package *plisql_compile_packageitem;

	/* pl_comp.c */
	int datums_alloc;

	/* pl_scanner.c */
	void *yylex_data;

	/* pl_funcs.c */
	PLiSQL_nsitem *ns_top;

	/* Begin - ReqID:SRS-PLSQL-GOTO */
	/* pl_subproc_function.c */
	void *plisql_saved_label_context[FUNC_MAX_NEST_LEVEL];
	/* pl_funcs.c */
	void *plisql_label_context;
	/* End - ReqID:SRS-PLSQL-GOTO */
} PLiSQL_compile_global_proper;


/*
 * package'var referenced by another function or package
 */
typedef struct PLiSQL_pkg_datum
{
	int		 dtype;
	int		 dno;
	Oid		 pkgoid;  /* not used */
	char		*refname;
	int		 lineno;
	PackageCacheItem *item;
	PLiSQL_datum *pkgvar;	 /* reference to package'var */
} PLiSQL_pkg_datum;


extern PLiSQL_package*plisql_compile_packageitem;
extern int plisql_curr_global_proper_level;
extern void *plisql_package_parse(ParseState *parsestate, PackageCacheItem *item,
									List *names, int name_start,
									package_parse_flags flags, Oid *basetypeid,
									/* Begin - ReqID:SRS-PLSQL-TYPE */
									int32 *basetypmod,
									/* End - ReqID:SRS-PLSQL-TYPE */
									int *entry_type,
									List **fargs, /* return value */
									List *fargnames,
									int nargs,
									Oid *argtypes,
									bool expand_variadic,
									bool expand_defaults,
									Oid *funcid,	/* return value */
									Oid *rettype,	/* return value */
									bool *retset,	/* return value */
									int *nvargs,	/* return value */
									Oid *vatype,	/* return value */
									Oid **true_typeids, /* return value */
									List **argdefaults, /* return value */
									void **pfunc,		/* return value */
									bool missing_ok,
									bool *is_explicit_cursor, /* ReqID:SRS-PLSQL-CURSOR */
									bool has_udt_type);	/* ReqID:SRS-PLSQL-RECORD */

extern void plisql_free_package_function(PackageCacheItem *item);
extern void plisql_free_packagelist(List *pkglist);
extern PkgEntry *plisql_parse_package_entry(const char *refname, int lineno,
					 List *idents,
					 bool add2namespace,
					 bool missing_ok);
extern PLiSQL_type *plisql_parse_package_type(TypeName *typeName,
														parse_pkgtype_flag flag,
														bool missing_ok);
extern PLiSQL_function *plisql_get_package_func(FunctionCallInfo fcinfo,  bool forValidator);
extern void plisql_remove_function_relations(PLiSQL_function *func);
extern void plisql_check_package_refcursor_var(int sqlvarno);
extern int plisql_get_package_last_globaldno(int sqlstate_dno, int sqlerrm_dno);
extern PLiSQL_package_status plisql_set_package_compile_status(void);
extern void plisql_close_package_cursorvar(PLiSQL_var *var);
extern PackageCacheItem *get_current_compile_package(PackageCacheKey key);
extern Oid plisql_top_functin_oid(FuncExpr *funcexpr, bool *is_package); /* BUG#M000088 */
extern void plisql_package_validator(Oid objectid, bool is_body);
extern PackageCacheItem *plisql_package_handle(Oid objectid, bool is_body);
extern PLiSQL_datum *plisql_get_datum(PLiSQL_execstate *estste, PLiSQL_datum *pkgdatum);
extern MemoryContext plisql_get_relevantContext(PLiSQL_variable *var, /* BUG#M0000167 */
								MemoryContext orig);
extern PLiSQL_datum *get_package_datum_bydno(PLiSQL_execstate *estate,
									Oid pkgoid, int dno);
extern bool is_package_global_var(PLiSQL_variable *var); /* BUG#M0000167 */
extern bool is_const_datum(PLiSQL_execstate *estate, PLiSQL_datum *datum);
extern void plisql_expand_rec_field(PLiSQL_rec *rec);
extern void plisql_check_subproc_define(PLiSQL_function *function);
extern void plisql_compile_package_error_callback(void *arg);
extern Datum package_datumTransfer(Datum value, bool typByVal,
							int typLen, bool isnull);
extern bool is_compile_standard_package(void);
/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
extern bool is_compile_dbms_standard_package(void);
/* End - ReqID:SRS-PLSQL-EXCEPTION */

extern void plisql_get_subprocs_from_package(Oid pkgoid, TupleDesc tupdesc,
											Tuplestorestate *tupstore);
extern int plisql_get_subproc_arg_info (FuncExpr *fexpr,
								Oid **p_argtypes,
								char ***p_argnames,
								char **p_argmodes);
extern char plisql_get_subproc_prokind (FuncExpr *fexpr);
extern bool plisql_subproc_should_change_return_type(FuncExpr *fexpr,
								Oid *rettype,
								int32 *typmod,
								Oid *collationoid);
extern bool is_row_record_datum(PLiSQL_datum *datum);

/* Begin - BUG#M000088 */
extern void release_package_func_usecount(FunctionCallInfo fcinfo);
/* End - BUG#M000088 */

/* Begin - BUG#M0000141 */
extern char plisql_get_subproc_parallel_enable(FuncExpr *fexpr);
/* End - BUG#M0000141 */

/* Begin - BUG#M0000432 */
extern char plisql_get_subproc_deterministic(FuncExpr *fexpr);
/* End - BUG#M0000432 */

/* Begin - ReqID:SRS-PLSQL-CURSOR */
extern void plisql_push_compile_global_proper(void);
extern void plisql_pop_compile_global_proper(void);
extern void plisql_check_package_explicit_cursor_define(PLiSQL_package *psource);
extern void plisql_handle_package_explicit_cursor_retdesc(int globdno);
/* End - ReqID:SRS-PLSQL-CURSOR */

/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
extern TupleDesc get_plisql_rec_tupdesc(PLiSQL_rec *rec);
/* End - ReqID:SRS-PLSQL-EXCEPTION */

#endif

