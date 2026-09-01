/*-------------------------------------------------------------------------
 *
 * funcapi.h
 *	  Definitions for functions which return composite type and/or sets
 *	  or work on VARIADIC inputs.
 *
 * This file must be included by all Postgres modules that either define
 * or call FUNCAPI-callable functions or macros.
 *
 *
 * Copyright (c) 2002-2021, PostgreSQL Global Development Group
 *
 * src/include/funcapi.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef FUNCAPI_H
#define FUNCAPI_H

#include "access/tupdesc.h"
#include "executor/executor.h"
#include "executor/tuptable.h"
/* Begin - ReqID:SRS-SQL-PACKAGE */
#include "utils/packagecache.h"
#include "parser/parse_node.h"
/* End - ReqID:SRS-SQL-PACKAGE */
#include "parser/parse_package.h" /* OA#00002227 */
#include "fmgr.h"
/* Begin - ReqID:SRS-SQL-CustomObject */
#include "parser/parse_func.h"
/* End - ReqID:SRS-SQL-CustomObject */

/*-------------------------------------------------------------------------
 *	Support to ease writing Functions returning composite types
 *-------------------------------------------------------------------------
 *
 * This struct holds arrays of individual attribute information
 * needed to create a tuple from raw C strings. It also requires
 * a copy of the TupleDesc. The information carried here
 * is derived from the TupleDesc, but it is stored here to
 * avoid redundant cpu cycles on each call to an SRF.
 */
typedef struct AttInMetadata
{
	/* full TupleDesc */
	TupleDesc	tupdesc;

	/* array of attribute type input function finfo */
	FmgrInfo   *attinfuncs;

	/* array of attribute type i/o parameter OIDs */
	Oid		   *attioparams;

	/* array of attribute typmod */
	int32	   *atttypmods;
} AttInMetadata;

/*-------------------------------------------------------------------------
 *		Support struct to ease writing Set Returning Functions (SRFs)
 *-------------------------------------------------------------------------
 *
 * This struct holds function context for Set Returning Functions.
 * Use fn_extra to hold a pointer to it across calls
 */
typedef struct FuncCallContext
{
	/*
	 * Number of times we've been called before
	 *
	 * call_cntr is initialized to 0 for you by SRF_FIRSTCALL_INIT(), and
	 * incremented for you every time SRF_RETURN_NEXT() is called.
	 */
	uint64		call_cntr;

	/*
	 * OPTIONAL maximum number of calls
	 *
	 * max_calls is here for convenience only and setting it is optional. If
	 * not set, you must provide alternative means to know when the function
	 * is done.
	 */
	uint64		max_calls;

	/*
	 * OPTIONAL pointer to miscellaneous user-provided context information
	 *
	 * user_fctx is for use as a pointer to your own struct to retain
	 * arbitrary context information between calls of your function.
	 */
	void	   *user_fctx;

	/*
	 * OPTIONAL pointer to struct containing attribute type input metadata
	 *
	 * attinmeta is for use when returning tuples (i.e. composite data types)
	 * and is not used when returning base data types. It is only needed if
	 * you intend to use BuildTupleFromCStrings() to create the return tuple.
	 */
	AttInMetadata *attinmeta;

	/*
	 * memory context used for structures that must live for multiple calls
	 *
	 * multi_call_memory_ctx is set by SRF_FIRSTCALL_INIT() for you, and used
	 * by SRF_RETURN_DONE() for cleanup. It is the most appropriate memory
	 * context for any memory that is to be reused across multiple calls of
	 * the SRF.
	 */
	MemoryContext multi_call_memory_ctx;

	/*
	 * OPTIONAL pointer to struct containing tuple description
	 *
	 * tuple_desc is for use when returning tuples (i.e. composite data types)
	 * and is only needed if you are going to build the tuples with
	 * heap_form_tuple() rather than with BuildTupleFromCStrings(). Note that
	 * the TupleDesc pointer stored here should usually have been run through
	 * BlessTupleDesc() first.
	 */
	TupleDesc	tuple_desc;

} FuncCallContext;

/*----------
 *	Support to ease writing functions returning composite types
 *
 * External declarations:
 * get_call_result_type:
 *		Given a function's call info record, determine the kind of datatype
 *		it is supposed to return.  If resultTypeId isn't NULL, *resultTypeId
 *		receives the actual datatype OID (this is mainly useful for scalar
 *		result types).  If resultTupleDesc isn't NULL, *resultTupleDesc
 *		receives a pointer to a TupleDesc when the result is of a composite
 *		type, or NULL when it's a scalar result or the rowtype could not be
 *		determined.  NB: the tupledesc should be copied if it is to be
 *		accessed over a long period.
 * get_expr_result_type:
 *		Given an expression node, return the same info as for
 *		get_call_result_type.  Note: the cases in which rowtypes cannot be
 *		determined are different from the cases for get_call_result_type.
 * get_func_result_type:
 *		Given only a function's OID, return the same info as for
 *		get_call_result_type.  Note: the cases in which rowtypes cannot be
 *		determined are different from the cases for get_call_result_type.
 *		Do *not* use this if you can use one of the others.
 *
 * See also get_expr_result_tupdesc(), which is a convenient wrapper around
 * get_expr_result_type() for use when the caller only cares about
 * determinable-rowtype cases.
 *----------
 */

/* Type categories for get_call_result_type and siblings */
typedef enum TypeFuncClass
{
	TYPEFUNC_SCALAR,			/* scalar result type */
	TYPEFUNC_COMPOSITE,			/* determinable rowtype result */
	TYPEFUNC_COMPOSITE_DOMAIN,	/* domain over determinable rowtype result */
	TYPEFUNC_RECORD,			/* indeterminate rowtype result */
	TYPEFUNC_OTHER				/* bogus type, eg pseudotype */
} TypeFuncClass;

/* Begin - ReqID:SRS-SQL-CustomObject */
typedef struct ObjectTypeSpec ObjectTypeSpec;
typedef struct ObjectTypeBody ObjectTypeBody;
/* End - ReqID:SRS-SQL-CustomObject */

/* Begin - SRS-PLSQL-SUBPROC*/
/*
 * function come from subproc
 * we should use function pointer to
 * get its informations, because its struct
 * is PLiSQL_xxx
 */
typedef struct
{
	TupleDesc (*get_internal_func_result_tupdesc) (FuncExpr *fexpr);
	char* (*get_internal_func_name) (FuncExpr *fexpr);
	TypeFuncClass (*get_internal_func_result_type) (FuncExpr *fexpr,
						 ReturnSetInfo *rsinfo,
						 Oid *resultTypeId,
						 TupleDesc *resultTupleDesc);
	List* (*get_internal_func_outargs) (FuncExpr *fexpr);
	char *(*get_inernal_func_result_name) (FuncExpr *fexpr);
	void (*compile_inline_internal)(char *proc_source);
	/* Begin - ReqID:SRS-SQL-PACKAGE */
	void (*package_validator) (Oid objectId, bool is_body);
	PackageCacheItem*  (*package_handle) (Oid objectId, bool is_body);
	/* Begin - ReqID:SRS-SQL-CustomObject */
	void *(*object_type_handle) (Oid objectId, bool is_body);
	void (*object_type_validator) (Oid objectId, bool is_body);
	ObjectTypeSpec *(*object_type_parse_spec) (const char *type_name,
											 const char *objsrc);
	ObjectTypeBody *(*object_type_parse_body) (const char *type_name,
											 const char *bodysrc);
	FuncDetailCode (*object_type_lookup) (ParseState *pstate,
										  void *object_source,
										  const char *method_name,
										  List **fargs,
										  bool proc_call,
										  List *fargnames,
										  int nargs,
										  Oid *argtypes,
										  Oid *funcid,
										  Oid *rettype,
										  bool *retset,
										  int *nvargs,
										  Oid *vatype,
										  Oid **true_typeids,
										  List **argdefaults,
										  bool has_udt_type);
	/* End - ReqID:SRS-SQL-CustomObject */
	void* (*package_parse) (ParseState *pstate,
							PackageCacheItem *item,
							List *names,
							int name_start,
							package_parse_flags flag,
							Oid *basetypeid, /* return value */
							/* Begin - ReqID:SRS-PLSQL-TYPE */
							int32 *basetypmod,
							/* End - ReqID:SRS-PLSQL-TYPE */
							int *entry_type, /* return value */
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
							bool has_udt_type);/* ReqID:SRS-PLSQL-RECORD */
	void (*package_free) (PackageCacheItem *item);
	void (*package_free_list) (List *pkglist);
	Oid (*get_top_function_id) (FuncExpr *expr, bool *is_package);
	void (*get_subprocs_from_package) (Oid pkgoid, TupleDesc tupdesc,
									Tuplestorestate *tupstore);
	int (*get_subproc_arg_info) (FuncExpr *fexpr,
								Oid **p_argtypes,
								char ***p_argnames,
								char **p_argmodes);
	char (*get_subproc_prokind) (FuncExpr *fexpr);
	bool (*subproc_should_change_return_type) (FuncExpr *fexpr,
								Oid *rettype,
								int32 *typmod,
								Oid *collationoid);
	/* End - ReqID:SRS-SQL-PACKAGE */
	/* Begin - ReqID:SRS-PLSQL-TYPE */
	void (*function_free) (Oid funcOid);
	/* End - ReqID:SRS-PLSQL-TYPE */
	/* Begin - BUG#M0000141 */
	char (*get_subproc_parallel_enable) (FuncExpr *expr);
	/* End - BUG#M0000141 */
	/* Begin - BUG#M0000432 */
	char (*get_subproc_deterministic) (FuncExpr *expr);
	/* ENd - BUG#M0000432 */
	/* Begin - OA#00002227 */
	PkgGlobalVarParam* (*build_gparam_from_package) (PkgVar *pvar);
	void (*get_package_globalvar_value) (PkgGlobalVarParam *gparam,
										Datum *resvalue,
										bool *isnull);
	/* End - OA#00002227*/
	bool isload;
} PLiSQL_funcs_call;

extern PGDLLIMPORT PLiSQL_funcs_call plisql_internal_funcs;

/* End - SRS-PLSQL-SUBPROC */

/* Begin - ReqID:SRS-PLSQL-CURSOR */
/*
 * function come from ivorysql_ora/src/orarefcursor
 * but it can be used in plisql so we add there
 */
typedef enum UdtExtensionLoadState
{
	UNLOADED,			/* not loaded */
	LOADED, 			/* loaded */
	LOAD_FAILED 		/* failure */
} UdtExtensionLoadState;

typedef struct
{
	char*  (*DatumGetOraRefCursorPortal) (Datum datum);
	bool (*OraRefcursorIsnull) (Datum datum);
	void* (*DatumGetOraRefCursorType) (Datum datum);
	Datum (*init_orarefcursor_value) (void *type, const char *name);
	void (*OraRefcursorSetType) (void *type, Datum datum);

	UdtExtensionLoadState loadstate;	/* orarefcursor datatype functions loading state */
} orarefcursor_funcs_call;

extern PGDLLIMPORT orarefcursor_funcs_call orarefcursor_funcs;
/* End - ReqID:SRS-PLSQL-CURSOR */

/* Begin - ReqID:SRS-PLSQL-Collection */
typedef struct
{
	Datum (*InitCollectypeVar) (Datum udttype_info);
	Datum (*CreateCollectInfo)(const char *typname,
				   Oid typoid,
				   Oid sqlcollectionoid, /* ReqID:SRS-SQL-Collection */
				   int32 typmod,
				   char typtype,
				   bool typbyval,
				   int typlen,
				   Datum typkey,
				   Datum typdatatype,
				   int limit,
				   bool datanotnull);
	Datum (*CopyCollectypeDatum)(Datum src_value);
	Datum (*CollectypeElementRef)(Datum datum, int nSubscripts,
						Datum *indx, bool *isNull);
	bool (*CollectypeCastCheck)(Datum value, int32 reqtypmod);
	Datum (*GetCollectypeName)(Datum value);
	Datum (*GetCollectypeMode)(Datum value);
	void (*SetCollectValue)(Datum val, int nsubscripts,
					Datum *subscriptvals,
					int *nth,
					List **fieldnames,
					void *top_datatype,
					Datum assign_value,
					bool assign_value_isnull);
	MemoryContext (*GetCollectypeMcxt)(Datum value);
	/* Begin - ReqID:SRS-PLSQL-BULKCOLLECT */
	Datum (*InitBulkCollectData)(Datum datum,
						int nSubscripts,
						Datum *subscriptvals,
						List **fieldnames,
						Datum *keytypes,
						void *root_datatype,
						void *target_datatype,
						MemoryContext cxt);
	Datum (*InitBulkCollectParams)(int num, Datum *collect_data);
	void (*SetBulkCollectDestReceiverParams)(Datum dest, Datum bulk_params);
	Datum (*CreateBulkCollectReceiver)(void);
	/* End - ReqID:SRS-PLSQL-BULKCOLLECT */
	/* Begin - ReqID:SRS-PLSQL-FORALL */
	void (*SetBulkCollectDestReceiverInForall)(Datum dest);
	List* (*GetCollectAllValues)(Datum collect_value);
	List* (*GetCollectAllKeyValues)(Datum collect_value, bool has_between_and,
						int lower_bound, int upper_bound);
	/* End - ReqID:SRS-PLSQL-FORALL */
	/* Begin - ReqID:SRS-SQL-Collection */
	Oid (*GetSqlCollectypeOid)(Datum value);
	/* End - ReqID:SRS-SQL-Collection */

	UdtExtensionLoadState loadstate;	/* collection type functions loading state */
} collection_funcs_call;

extern PGDLLIMPORT collection_funcs_call collection_funcs;
/* End - ReqID:SRS-PLSQL-Collection */

extern TypeFuncClass get_call_result_type(FunctionCallInfo fcinfo,
										  Oid *resultTypeId,
										  TupleDesc *resultTupleDesc);
extern TypeFuncClass get_expr_result_type(Node *expr,
										  Oid *resultTypeId,
										  TupleDesc *resultTupleDesc);
extern TypeFuncClass get_func_result_type(Oid functionId,
										  Oid *resultTypeId,
										  TupleDesc *resultTupleDesc);

extern TupleDesc get_expr_result_tupdesc(Node *expr, bool noError);

extern bool resolve_polymorphic_argtypes(int numargs, Oid *argtypes,
										 char *argmodes,
										 Node *call_expr);

extern int	get_func_arg_info(HeapTuple procTup,
							  Oid **p_argtypes, char ***p_argnames,
							  char **p_argmodes);

extern int	get_func_input_arg_names(Datum proargnames, Datum proargmodes,
									 char ***arg_names);

extern int	get_func_trftypes(HeapTuple procTup, Oid **p_trftypes);
extern char *get_func_result_name(Oid functionId);

extern TupleDesc build_function_result_tupdesc_d(char prokind,
												 Datum proallargtypes,
												 Datum proargmodes,
												 Datum proargnames);
extern TupleDesc build_function_result_tupdesc_d_ext(char prokind,
													 Datum proallargtypes,
													 Datum proargmodes,
													 Datum proargnames,
													 bool intisnumber,		 /* BUG:M0000068 */
													 Datum proargtypenames, /* ReqID:SRS-SQL-PACKAGE */
													 Oid funcid /* BUG#Z9039 */);
/* End - ReqID:SRS-SQL-PACKAGE */
extern TupleDesc build_function_result_tupdesc_t(HeapTuple procTuple);
/* Begin - ReqID:SRS-SQL-FUNCOUT */
extern TupleDesc build_plisql_function_result_tupdesc_d(char prokind,
								Datum proallargtypes,
								Datum proargmodes,
								Datum proargnames,
								Oid prorettype,
								bool intisnumber, /* BUG:M0000068 */
								Datum proargtypenames, /* ReqID:SRS-SQL-PACKAGE */
								Oid funcid /* BUG#Z9039 */);
/* End - ReqID:SRS-SQL-FUNCOUT */

/* Begin - SRS-PLSQL-SUBPROC */
extern bool resolve_polymorphic_tupdesc(TupleDesc tupdesc,
										oidvector *declared_args,
										Node *call_expr);
extern TypeFuncClass get_type_func_class(Oid typid, Oid *base_typeid);
extern TupleDesc build_internal_function_result_tupdesc_t(FuncExpr *fexpr);
extern char	*get_internal_function_name(FuncExpr *fexpr);
extern TypeFuncClass get_internal_function_result_type(FuncExpr *fexpr,
						 ReturnSetInfo *rsinfo,
						 Oid *resultTypeId,
						 TupleDesc *resultTupleDesc);
extern TypeFuncClass external_get_type_func_class(Oid typid, Oid *base_typeid);
extern List *get_internal_function_outargs(FuncExpr *fexpr);
extern char *get_internal_function_result_name(FuncExpr *fexpr);

/* End - SRS-PLSQL-SUBPROC */


/* Begin - ReqID:SRS-SQL-FUNCOUT */
extern bool	func_should_change_return_type(Oid functionId, Oid *rettype,
								int32 *typmod, Oid *collationoid);
/* End - ReqID:SRS-SQL-FUNCOUT */

/* Begin - ReqID:SRS-SQL-PACKAGE */
extern void get_func_typename_info(HeapTuple procTup,
					char ***p_argtypeNames,
					char **rettypeName);
extern char get_subproc_kind(FuncExpr *fexpr);
extern int get_subproc_arg_info(FuncExpr *fexpr,
					Oid **p_argtypes,
					char ***p_argnames,
					char **p_argmodes);
extern bool subproc_should_change_return_type(FuncExpr *fexpr,
								Oid *rettype,
								int32 *typmod,
								Oid *collationoid);
/* End - ReqID:SRS-SQL-PACKAGE */

/* Begin - ReqID:SRS-PLSQL-TYPE */
extern char get_func_prostatus(HeapTuple procTup);
extern void change_func_prostatus(Oid funcOid, char prostatus);
/* End - ReqID:SRS-PLSQL-TYPE */


/* Begin - BUG#M0000141 */
extern char get_subproc_parallel_enable(FuncExpr *funcexpr);
/* End - BUG#M0000141 */

/* Begin - BUG#M0000432 */
extern char get_subproc_deterministic(FuncExpr *funcexpr);
/* End - BUG#M0000432 */



/*----------
 *	Support to ease writing functions returning composite types
 *
 * External declarations:
 * TupleDesc BlessTupleDesc(TupleDesc tupdesc) - "Bless" a completed tuple
 *		descriptor so that it can be used to return properly labeled tuples.
 *		You need to call this if you are going to use heap_form_tuple directly.
 *		TupleDescGetAttInMetadata does it for you, however, so no need to call
 *		it if you call TupleDescGetAttInMetadata.
 * AttInMetadata *TupleDescGetAttInMetadata(TupleDesc tupdesc) - Build an
 *		AttInMetadata struct based on the given TupleDesc. AttInMetadata can
 *		be used in conjunction with C strings to produce a properly formed
 *		tuple.
 * HeapTuple BuildTupleFromCStrings(AttInMetadata *attinmeta, char **values) -
 *		build a HeapTuple given user data in C string form. values is an array
 *		of C strings, one for each attribute of the return tuple.
 * Datum HeapTupleHeaderGetDatum(HeapTupleHeader tuple) - convert a
 *		HeapTupleHeader to a Datum.
 *
 * Macro declarations:
 * HeapTupleGetDatum(HeapTuple tuple) - convert a HeapTuple to a Datum.
 *
 * Obsolete routines and macros:
 * TupleDesc RelationNameGetTupleDesc(const char *relname) - Use to get a
 *		TupleDesc based on a named relation.
 * TupleDesc TypeGetTupleDesc(Oid typeoid, List *colaliases) - Use to get a
 *		TupleDesc based on a type OID.
 * TupleGetDatum(TupleTableSlot *slot, HeapTuple tuple) - get a Datum
 *		given a tuple and a slot.
 *----------
 */

#define HeapTupleGetDatum(tuple)		HeapTupleHeaderGetDatum((tuple)->t_data)
/* obsolete version of above */
#define TupleGetDatum(_slot, _tuple)	HeapTupleGetDatum(_tuple)

extern TupleDesc RelationNameGetTupleDesc(const char *relname);
extern TupleDesc TypeGetTupleDesc(Oid typeoid, List *colaliases);

/* from execTuples.c */
extern TupleDesc BlessTupleDesc(TupleDesc tupdesc);
extern AttInMetadata *TupleDescGetAttInMetadata(TupleDesc tupdesc);
extern HeapTuple BuildTupleFromCStrings(AttInMetadata *attinmeta, char **values);
extern Datum HeapTupleHeaderGetDatum(HeapTupleHeader tuple);


/*----------
 *		Support for Set Returning Functions (SRFs)
 *
 * The basic API for SRFs using ValuePerCall mode looks something like this:
 *
 * Datum
 * my_Set_Returning_Function(PG_FUNCTION_ARGS)
 * {
 *	FuncCallContext    *funcctx;
 *	Datum				result;
 *	MemoryContext		oldcontext;
 *	<user defined declarations>
 *
 *	if (SRF_IS_FIRSTCALL())
 *	{
 *		funcctx = SRF_FIRSTCALL_INIT();
 *		// switch context when allocating stuff to be used in later calls
 *		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
 *		<user defined code>
 *		<if returning composite>
 *			<build TupleDesc, and perhaps AttInMetadata>
 *		<endif returning composite>
 *		<user defined code>
 *		// return to original context when allocating transient memory
 *		MemoryContextSwitchTo(oldcontext);
 *	}
 *	<user defined code>
 *	funcctx = SRF_PERCALL_SETUP();
 *	<user defined code>
 *
 *	if (funcctx->call_cntr < funcctx->max_calls)
 *	{
 *		<user defined code>
 *		<obtain result Datum>
 *		SRF_RETURN_NEXT(funcctx, result);
 *	}
 *	else
 *		SRF_RETURN_DONE(funcctx);
 * }
 *
 * NOTE: there is no guarantee that a SRF using ValuePerCall mode will be
 * run to completion; for example, a query with LIMIT might stop short of
 * fetching all the rows.  Therefore, do not expect that you can do resource
 * cleanup just before SRF_RETURN_DONE().  You need not worry about releasing
 * memory allocated in multi_call_memory_ctx, but holding file descriptors or
 * other non-memory resources open across calls is a bug.  SRFs that need
 * such resources should not use these macros, but instead populate a
 * tuplestore during a single call, and return that using SFRM_Materialize
 * mode (see fmgr/README).  Alternatively, set up a callback to release
 * resources at query shutdown, using RegisterExprContextCallback().
 *
 *----------
 */

/* from funcapi.c */
extern FuncCallContext *init_MultiFuncCall(PG_FUNCTION_ARGS);
extern FuncCallContext *per_MultiFuncCall(PG_FUNCTION_ARGS);
extern void end_MultiFuncCall(PG_FUNCTION_ARGS, FuncCallContext *funcctx);

#define SRF_IS_FIRSTCALL() (fcinfo->flinfo->fn_extra == NULL)

#define SRF_FIRSTCALL_INIT() init_MultiFuncCall(fcinfo)

#define SRF_PERCALL_SETUP() per_MultiFuncCall(fcinfo)

#define SRF_RETURN_NEXT(_funcctx, _result) \
	do { \
		ReturnSetInfo	   *rsi; \
		(_funcctx)->call_cntr++; \
		rsi = (ReturnSetInfo *) fcinfo->resultinfo; \
		rsi->isDone = ExprMultipleResult; \
		PG_RETURN_DATUM(_result); \
	} while (0)

#define SRF_RETURN_NEXT_NULL(_funcctx) \
	do { \
		ReturnSetInfo	   *rsi; \
		(_funcctx)->call_cntr++; \
		rsi = (ReturnSetInfo *) fcinfo->resultinfo; \
		rsi->isDone = ExprMultipleResult; \
		PG_RETURN_NULL(); \
	} while (0)

#define  SRF_RETURN_DONE(_funcctx) \
	do { \
		ReturnSetInfo	   *rsi; \
		end_MultiFuncCall(fcinfo, _funcctx); \
		rsi = (ReturnSetInfo *) fcinfo->resultinfo; \
		rsi->isDone = ExprEndResult; \
		PG_RETURN_NULL(); \
	} while (0)

/*----------
 *	Support to ease writing of functions dealing with VARIADIC inputs
 *----------
 *
 * This function extracts a set of argument values, types and NULL markers
 * for a given input function. This returns a set of data:
 * - **values includes the set of Datum values extracted.
 * - **types the data type OID for each element.
 * - **nulls tracks if an element is NULL.
 *
 * variadic_start indicates the argument number where the VARIADIC argument
 * starts.
 * convert_unknown set to true will enforce the conversion of arguments
 * with unknown data type to text.
 *
 * The return result is the number of elements stored, or -1 in the case of
 * "VARIADIC NULL".
 */
extern int	extract_variadic_args(FunctionCallInfo fcinfo, int variadic_start,
								  bool convert_unknown, Datum **values,
								  Oid **types, bool **nulls);

/* Begin - ReqID:SRS-PLSQL-DBMS_OUTPUT */
typedef void (*dbmsoutput_sendbuffer_hook_type)(void);
typedef void (*dbmsoutput_errorhandler_hook_type)(void); /* BUG#Z5656 */
typedef void (*dbmsoutput_severoutput_hook_type)(bool, int32);

extern PGDLLIMPORT dbmsoutput_sendbuffer_hook_type dbmsoutput_sendbuffer_hook;
extern PGDLLIMPORT dbmsoutput_errorhandler_hook_type dbmsoutput_errorhandler_hook;
extern PGDLLIMPORT dbmsoutput_severoutput_hook_type dbmsoutput_severoutput_hook;
extern PGDLLIMPORT bool enable_serveroutput;
/* End - ReqID:SRS-PLSQL-DBMS_OUTPUT */

#endif							/* FUNCAPI_H */
