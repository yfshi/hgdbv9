/*-------------------------------------------------------------------------
 *
 * parse_object_type.h
 *	  Oracle object type method lookup support.
 *
 *-------------------------------------------------------------------------
 */

#ifndef PARSE_OBJECT_TYPE_H
#define PARSE_OBJECT_TYPE_H

#include "parser/parse_node.h"
#include "parser/parse_func.h"

extern FuncDetailCode LookupObjectTypeFunc(ParseState *pstate,
										   List *funcname,
										   List **fargs,
										   List *fargnames,
										   int nargs,
										   Oid *argtypes,
										   bool is_object_method,
										   bool expand_variadic,
										   bool expand_defaults,
										   bool proc_call,
										   bool is_column_call,
										   Oid *funcid,
										   Oid *rettype,
										   bool *retset,
										   int *nvargs,
										   Oid *vatype,
										   Oid **true_typeids,
										   List **argdefaults,
										   void **pfunc,
										   Oid *objoid,
										   bool has_udt_type);
extern FuncDetailCode LookupObjectTypeConstructor(ParseState *pstate,
												  List *funcname,
												  List **fargs,
												  List *fargnames,
												  int nargs,
												  Oid *argtypes,
												  bool proc_call,
												  Oid *funcid,
												  Oid *rettype,
												  bool *retset,
												  int *nvargs,
												  Oid *vatype,
												  Oid **true_typeids,
												  List **argdefaults,
												  void **pfunc,
												  Oid *objoid,
												  bool has_udt_type);

#endif
