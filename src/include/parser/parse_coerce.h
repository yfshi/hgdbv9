/*-------------------------------------------------------------------------
 *
 * parse_coerce.h
 *	Routines for type coercion.
 *
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/parser/parse_coerce.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PARSE_COERCE_H
#define PARSE_COERCE_H

#include "fmgr.h"
#include "parser/parse_node.h"

/* START BUG#M0000285 */
typedef Datum (* ora_raw_input_hook_type)(PG_FUNCTION_ARGS);
/* END BUG#M0000285 */

extern PGDLLIMPORT ora_raw_input_hook_type ora_raw_input_hook;

/* Type categories (see TYPCATEGORY_xxx symbols in catalog/pg_type.h) */
typedef char TYPCATEGORY;

/* Result codes for find_coercion_pathway */
typedef enum CoercionPathType
{
	COERCION_PATH_NONE,			/* failed to find any coercion pathway */
	COERCION_PATH_FUNC,			/* apply the specified coercion function */
	COERCION_PATH_RELABELTYPE,	/* binary-compatible cast, no function */
	COERCION_PATH_ARRAYCOERCE,	/* need an ArrayCoerceExpr node */
	COERCION_PATH_COERCEVIAIO	/* need a CoerceViaIO node */
} CoercionPathType;


extern bool IsBinaryCoercible(Oid srctype, Oid targettype);
extern bool IsPreferredType(TYPCATEGORY category, Oid type);
extern TYPCATEGORY TypeCategory(Oid type);

extern Node *coerce_to_target_type(ParseState *pstate,
								   Node *expr, Oid exprtype,
								   Oid targettype, int32 targettypmod,
								   CoercionContext ccontext,
								   CoercionForm cformat,
								   int location);
extern bool can_coerce_type(int nargs, const Oid *input_typeids, const Oid *target_typeids,
							CoercionContext ccontext);
extern Node *coerce_type(ParseState *pstate, Node *node,
						 Oid inputTypeId, Oid targetTypeId, int32 targetTypeMod,
						 CoercionContext ccontext, CoercionForm cformat, int location);
extern Node *coerce_to_domain(Node *arg, Oid baseTypeId, int32 baseTypeMod,
							  Oid typeId,
							  CoercionContext ccontext, CoercionForm cformat, int location,
							  bool hideInputCoercion);

extern Node *coerce_to_boolean(ParseState *pstate, Node *node,
							   const char *constructName);
extern Node *coerce_to_specific_type(ParseState *pstate, Node *node,
									 Oid targetTypeId,
									 const char *constructName);

extern Node *coerce_to_specific_type_typmod(ParseState *pstate, Node *node,
											Oid targetTypeId, int32 targetTypmod,
											const char *constructName);

extern int	parser_coercion_errposition(ParseState *pstate,
										int coerce_location,
										Node *input_expr);

extern Oid	select_common_type(ParseState *pstate, List *exprs,
							   const char *context, Node **which_expr);
/* Begin - BUGID:0000536 */
extern Oid select_common_type_for_nvl(ParseState *pstate, List *exprs,
				   const char *context, Node **which_expr);
/* End - BUGID:0000536 */

extern Oid select_common_type_for_nvl2(ParseState *pstate, List *exprs,
				   const char *context, Node **which_expr); /* BUG#Z4592 */

extern Node *coerce_to_common_type(ParseState *pstate, Node *node,
								   Oid targetTypeId,
								   const char *context);
extern bool verify_common_type(Oid common_type, List *exprs);

extern int32 select_common_typmod(ParseState *pstate, List *exprs, Oid common_type);

extern bool check_generic_type_consistency(const Oid *actual_arg_types,
										   const Oid *declared_arg_types,
										   int nargs);
extern Oid	enforce_generic_type_consistency(const Oid *actual_arg_types,
											 Oid *declared_arg_types,
											 int nargs,
											 Oid rettype,
											 bool allow_poly);

extern char *check_valid_polymorphic_signature(Oid ret_type,
											   const Oid *declared_arg_types,
											   int nargs);
extern char *check_valid_internal_signature(Oid ret_type,
											const Oid *declared_arg_types,
											int nargs);

extern CoercionPathType find_coercion_pathway(Oid targetTypeId,
											  Oid sourceTypeId,
											  CoercionContext ccontext,
											  Oid *funcid);
extern CoercionPathType find_typmod_coercion_function(Oid typeId,
													  Oid *funcid);

/* Begin - ReqID:SRS-MYSQL-DATETYPE */
extern int32 Num2Mysdate(Const *con, Oid inputTypeId);
extern int32 Str2Mysdate(Const *con, Const *newcon);
/* End - ReqID:SRS-MYSQL-DATETYPE */

/* Begin - ReqID:SRS-MYSQL-DATETYPE */
extern int64 Num2Mysdatetime(Const *con, Oid inputTypeId);
extern int64 Str2Mysdatetime(Const *con, Const *newcon);
extern Datum Num2Mysdatetimes(Const *con, Oid inputTypeId, int dec, bool is_insert);
extern Datum Str2Mysdatetimes(Const *con, Const *newcon, int dec, bool is_insert);
/* End - ReqID:SRS-MYSQL-DATETYPE */

/* Begin - ReqID:SRS-MYSQL-TIMESTAMPTYPE */
extern int64 Num2Mystimestamp(Const *con, Oid inputTypeId);
extern int64 Str2Mystimestamp(Const *con, Const *newcon);
extern Datum Num2Mystimestamps(Const *con, Oid inputTypeId, int dec, bool is_insert);
extern Datum Str2Mystimestamps(Const *con, Const *newcon, int dec, bool is_insert);

/* End - ReqID:SRS-MYSQL-TIMESTAMPTYPE */

/* Begin - ReqID:SRS-MYSQL-TIMETYPE */
extern int64 Num2Mystime(Const *con, Oid inputTypeId, bool is_insert);
extern int64 Str2Mystime(Const *con, Const *newcon, bool is_insert);
extern Datum Num2Mystimes(Const *con, Oid inputTypeId, int dec, bool is_insert);
extern Datum Str2Mystimes(Const *con, Const *newcon, int dec, bool is_insert);

/* End - ReqID:SRS-MYSQL-TIMETYPE */

/* Begin - ReqID:SRS-MYSQL-NUMBERICTYPE-UINT4 */
extern uint64 Num2Mysuint4(Const *con, Oid inputTypeId);
extern uint64 Str2Mysuint4(Const *con, Const *newcon);
/* End - ReqID:SRS-MYSQL-NUMBERICTYPE-UINT4 */

/* Begin - ReqID:SRS-MYSQL-NUMBERICTYPE-UINT8 */
extern uint64 pg_atoi_uint8_check(const char *s,int c);
extern uint64 Num2Mysuint8(Const *con, Oid inputTypeId);
extern uint64 Str2Mysuint8(Const *con, Const *newcon);
/* End - ReqID:SRS-MYSQL-NUMBERICTYPE-UINT8 */

#endif							/* PARSE_COERCE_H */
