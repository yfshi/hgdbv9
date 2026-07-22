/*-------------------------------------------------------------------------
 *
 * pl_object_type.h
 *	  Oracle object type spec parsing support.
 *
 *-------------------------------------------------------------------------
 */

#ifndef PL_OBJECT_TYPE_H
#define PL_OBJECT_TYPE_H

#include "commands/objecttypecmds.h"
#include "parser/parse_func.h"
#include "parser/parse_node.h"
#include "plisql.h"

extern ObjectTypeSpec *plisql_parse_object_type_spec(const char *type_name,
											 const char *objsrc);
extern ObjectTypeBody *plisql_parse_object_type_body(const char *type_name,
											 const char *bodysrc);
extern FuncDetailCode plisql_object_type_lookup(ParseState *pstate,
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
extern void *plisql_object_type_handle(Oid objectid, bool is_body);
extern void plisql_object_type_validator(Oid objectid, bool is_body);
extern PLiSQL_function *plisql_get_object_method_func(FunctionCallInfo fcinfo,
													  bool forValidator);
extern void release_object_method_usecount(FunctionCallInfo fcinfo);

#endif
