/*-------------------------------------------------------------------------
 *
 * File:pl_udt_type.h
 *
 * Abstract:
 *		Definitions for the PLISQL
 *			  procedural language user define type
 *
 * ReqID:SRS-PLSQL-CURSOR
 *
 * Authored by dwdai@highgo.com,20230306.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  src/pl/plisql/src/pl_udt_type.h
 *
 *-------------------------------------------------------------------------
 */

#ifndef PL_UDT_TYPE_H
#define PL_UDT_TYPE_H
#include "plisql.h"
#include "access/tupdesc.h"
#include "parser/parse_node.h"

typedef struct PLiSQL_datum_udt_type
{
	/* PLiSQL_datum attribute */
	PLiSQL_datum_type dtype;
	int			dno;
	Oid			pkgoid;
	/* end PLiSQL_datum attribute */
	PLiSQL_type		*udt_type;
} PLiSQL_datum_udt_type;

typedef struct PLiSQL_ref_cursor_type
{
	PLiSQL_type	type;		/* type base type */
	int			lineno;		/* line num */
	struct PLiSQL_type *rettype;	/* cursor type return type */
	TupleDesc	rettype_tupdesc;	/* cursor type return type tupdesc */
} PLiSQL_ref_cursor_type;


typedef struct PLiSQL_explicit_cursor_type
{
	PLiSQL_type type;	/* type base type */
	TupleDesc	retdesc;	/* result tupledesc  */
} PLiSQL_explicit_cursor_type;

/*
 * used for cursor attribute like xxx%isoepn
 */
typedef struct PLiSQL_attribute
{
	PLiSQL_datum_type dtype;
	int			dno;
	/* Begin - ReqID:SRS-SQL-PACKAGE */
	Oid			pkgoid;
	/* End - ReqID:SRS-SQL-PACKAGE */
	char	   *fieldname;		/* name of field */
	PLiSQL_type *datatype;
	int			attributeparentno;	/* dno of parent cursor */
	Datum  value;
	bool    isnull;
	bool    freeval;
	bool    get_from_portal; /* should check portal */
} PLiSQL_attribute;

typedef enum PLiSQL_attribute_type
{
	PLISQL_ATTRIBUTE_ISOPEN = 0, /* bug#M364 */
	PLISQL_ATTRIBUTE_FOUND,
	PLISQL_ATTRIBUTE_NOTFOUND,
	PLISQL_ATTRIBUTE_ROWCOUNT
} PLiSQL_attribute_type;

#define PLISQL_SUPPORT_ATTRIBUTE_NUM 	PLISQL_ATTRIBUTE_ROWCOUNT

/* Begin - ReqID:SRS-PLSQL-RECORD */
/*
 * User defined record type.
 */
typedef struct PLiSQL_ora_record_argitem
{
	char		*argname;
	PLiSQL_type	*type;
	bool		notnull;
	PLiSQL_expr	*defexpr;
} PLiSQL_ora_record_argitem;

typedef struct PLiSQL_ora_record_type
{
	PLiSQL_type type;	/* type base type */
	List		*args;	/* ora record argtype, a list of PLSQL_ora_record_argitem */
} PLiSQL_ora_record_type;
/* End - ReqID:SRS-PLSQL-RECORD */

/* Begin - ReqID:SRS-PLSQL-Collection */
typedef struct PLiSQL_ora_collection_type
{
	PLiSQL_type 	type;			/* collection type base type */
	PLiSQL_type		*typkey;		/* key type */
	PLiSQL_type     *typdatatype;	/* value type */
	int 			limit;			/* max limit number of varray elements */
} PLiSQL_ora_collection_type;
/* End - ReqID:SRS-PLSQL-Collection */

/* Begin - BUG#Z1322 */
/* The maximum number of layers that record can nest is 100 */
#define PLISQL_MAXNUM_NESTED_RECORD		100
/* End - BUG#Z1322 */

/* Begin - ReqID:SRS-PLSQL-SUBTYPE */
typedef struct PLiSQL_subtype_type
{
	PLiSQL_type type;	/* type base type */
	bool	setRange;	/* subtype has range constraint? */
	int		lower;		/* used for range constraint, int32 */
	int 	upper;		/* used for range constraint, int32 */
} PLiSQL_subtype_type;
/* End - ReqID:SRS-PLSQL-SUBTYPE */

extern int plisql_check_cursor_define(PLiSQL_var *var,
							PLiSQL_type *rettype,
							List *arglist);
extern int plisql_build_cursor_args(List *arglist, int lineno);
extern int plisql_buld_rows_from_datums(int *datum_dnos, int ndatums);
extern bool check_all_args_has_default(List *arglist);
extern char *get_args_default_query(List *arglist, int argno);
extern void plisql_build_udt_cursor_type(const char *refname,
										PLiSQL_type *rettype,
										int lineno,
										bool add2namespace);
extern void plisql_register_orarefcursor_func(void);
extern void plisql_build_cursor_attribute(PLiSQL_var *cursor);
extern void plisql_build_package_local_cursor_attribute(PLiSQL_var *cursor,
																		bool add2namespace,
																		const char *refname,
																		int lineno);
extern bool is_sql_cursor(PLiSQL_execstate *estate, PLiSQL_var *var);
extern bool is_oracursor_datum(PLiSQL_datum *datum);
extern Datum plisql_get_cursor_attribute(PLiSQL_execstate *estate,
										PLiSQL_var *cursor,
										PLiSQL_attribute_type attr_type,
										bool *isnull);
extern bool is_implicut_cursor_or_attribute(PLiSQL_execstate *estate,
										int dno);
extern TupleDesc plisql_get_rowtype_tupledesc(PLiSQL_type *rowtype);

extern void exec_check_assign_cursor_value(PLiSQL_execstate *estate,
										PLiSQL_var *target,
										Datum value, bool isNull,
										Oid valtype, int32 valtypmod,
										bool missok);
extern PLiSQL_type *plisql_parse_udt_rowtype(const char *ident, bool byval);
extern PLiSQL_type *plisql_return_ref_cursor_rowtype(PLiSQL_type *rowtype);
extern PLiSQL_rec *plisql_build_record_for_cursorrowtype(const char *refname,
										int lineno,
										PLiSQL_type *dtype,
										bool add2namespace);
extern TupleDesc plisql_get_cursor_tupdesc_incompile(PLiSQL_expr *cursor_expr,
													int cursor_options);
extern PLiSQL_explicit_cursor_type *plisql_build_explicit_cursor_type(TupleDesc retdesc);
extern bool is_plisql_rowtype_record(PLiSQL_rec *rec);
extern void set_plisql_cursorvar_nsitem_typoid(PLiSQL_variable *variable,
											bool set_retype);
extern TupleDesc plisql_param_tupdesc_ref(ParseState *pstate, Param *param);

/* Begin - ReqID:SRS-PLSQL-RECORD */
extern PLiSQL_type *plisql_build_ora_rec_type(char *refname, int lineno, List *args);
extern PLiSQL_row * plisql_build_row_for_ora_record(const char *refname, int lineno,
					 PLiSQL_ora_record_type *dtype,
					 bool add2namespace);
extern PLiSQL_datum *plisql_parse_ora_rec_var(PLiSQL_datum **datums,
					PLiSQL_row *ora_rec, List *names,
					int match_start_loc, bool noError);
extern Node *plisql_ora_record_construct_ref(ParseState *pstate, List *recname, List *fargs,
				  FuncCall *fn, int location);
extern TupleDesc plisql_build_tupdesc_from_recordtype(PLiSQL_ora_record_type *ora_rec_type);
/* End - ReqID:SRS-PLSQL-RECORD */
/* Begin - BUG#Z1322 */
extern void plisql_check_maxnum_record_nest(int num_nested, int record_begin_pos);
/* End - BUG#Z1322 */
/* Begin - ReqID:SRS-PLSQL-SUBTYPE */
extern void plisql_build_subtype(char *subtypnme, int lineno, PLiSQL_type* btype_p, List *range_list, bool isNotNull);
/* End - ReqID:SRS-PLSQL-SUBTYPE */

/* Begin - ReqID:SRS-PLSQL-Collection */
extern PLiSQL_type * plisql_build_ora_collection_type(char *refname, int lineno,
							  Oid typoid,
							  char typtype,
							  PLiSQL_type *keytype,
							  PLiSQL_type *valuetype,
							  int varraylimit,
							  bool notnull);
extern Datum plisql_type_to_collectinfo(PLiSQL_type *type);
extern void exec_check_assign_collect_value(PLiSQL_var *target,
									Datum value,
									Oid valtype,
									int32 valtypmod);
extern PLiSQL_type * get_record_field_type(PLiSQL_type *data_type, char *name);
extern PLwdatum * plisql_get_ora_record_field(PLiSQL_datum *datum,
					List *idents,
					int nlen,
					int nth,
					int loc);
/* End - ReqID:SRS-PLSQL-Collection */
#endif

