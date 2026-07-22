/*-------------------------------------------------------------------------
 *
 * parse_object_type.c
 *	  Oracle object type method lookup support.
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "commands/objecttypecmds.h"
#include "funcapi.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "parser/parse_coerce.h"
#include "parser/parse_expr.h"
#include "parser/parse_object_type.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"
#include "utils/objecttypecache.h"
#include "utils/ora_compatible.h"
#include "utils/typcache.h"

static FuncDetailCode lookup_object_member_from_expr(ParseState *pstate,
													 Node *self_expr,
													 const char *method_name,
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
													 bool has_udt_type,
													 bool self_already_present);
static Node *build_object_constructor_self_expr(ParseState *pstate,
												Oid targetType,
												int location);
static List *build_name_prefix(List *funcname);
static void reject_object_constructor_method_target(Oid typoid,
													const char *method_name);

/*
 * LookupObjectTypeFunc
 *    Main entry point for resolving function calls as object type methods.
 *    Handles constructors, member methods, and static methods.
 */
FuncDetailCode
LookupObjectTypeFunc(ParseState *pstate,
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
					 bool has_udt_type)
{
	FuncDetailCode result = FUNCDETAIL_NOTFOUND;

	if (ORA_MODE != compatible_mode)
		return FUNCDETAIL_NOTFOUND;

	if (expand_variadic || !expand_defaults)
	{
		/* Keep the API surface aligned with other special lookups. */
	}
	(void) is_column_call;

	*objoid = InvalidOid;

	/*
	 * Try constructors first.  A constructor call shares the same surface
	 * syntax as an ordinary function call, so once we know the name resolves
	 * to an object type we need to give an explicit constructor declaration
	 * priority over the composite-type fallback in parse_func.c.
	 */
	result = LookupObjectTypeConstructor(pstate,
										 funcname,
										 fargs,
										 fargnames,
										 nargs,
										 argtypes,
										 proc_call,
										 funcid,
										 rettype,
										 retset,
										 nvargs,
										 vatype,
										 true_typeids,
										 argdefaults,
										 pfunc,
										 objoid,
										 has_udt_type);
	if (result != FUNCDETAIL_NOTFOUND)
		return result;

	if (is_object_method)
	{
		Node	   *self_expr;

		/*
		 * Grammar-marked object calls already carry the receiver expression as
		 * the first transformed argument.  Lookup only needs to validate that
		 * receiver type and resolve the method within the object cache.
		 */
		if (list_length(funcname) != 1 || nargs < 1 || *fargs == NIL)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("invalid object method call syntax")));

		self_expr = (Node *) linitial(*fargs);
		result = lookup_object_member_from_expr(pstate,
												self_expr,
												strVal(linitial(funcname)),
												fargs,
												fargnames,
												nargs,
												argtypes,
												proc_call,
												funcid,
												rettype,
												retset,
												nvargs,
												vatype,
												true_typeids,
												argdefaults,
												pfunc,
												objoid,
												has_udt_type,
												true);
		if (result == FUNCDETAIL_NOTFOUND)
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_FUNCTION),
					 errmsg("object type method \"%s.%s\" does not exist",
							format_type_be(exprType(self_expr)),
							strVal(linitial(funcname)))));
		return result;
	}

	if (list_length(funcname) >= 2)
	{
		List	   *prefix = build_name_prefix(funcname);
		Node	   *self_expr;

		/*
		 * A qualified name such as a.b.c can still be a member call if a.b
		 * resolves to a column/record/object expression and c is the member
		 * name.  Try that interpretation before treating the prefix as an
		 * object type name for static members.
		 */
		self_expr = TryTransformColumnRef(pstate, prefix);
		if (self_expr != NULL)
		{
			result = lookup_object_member_from_expr(pstate,
													self_expr,
													strVal(llast(funcname)),
													fargs,
													fargnames,
													nargs,
													argtypes,
													proc_call,
													funcid,
													rettype,
													retset,
													nvargs,
													vatype,
													true_typeids,
													argdefaults,
													pfunc,
													objoid,
													has_udt_type,
													false);
			if (result != FUNCDETAIL_NOTFOUND)
				return result;
		}

		*objoid = LookupObjectTypeByNames(prefix, true);
		if (OidIsValid(*objoid))
		{
			ObjectTypeCacheItem *item;

			if (strcmp(strVal(llast(prefix)), strVal(llast(funcname))) == 0)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("constructor \"%s\" cannot be invoked with object method syntax",
								strVal(llast(funcname)))));

			item = ObjectTypeHandle(*objoid, false);
			if (item != NULL && plisql_internal_funcs.object_type_lookup != NULL)
			{
				result = plisql_internal_funcs.object_type_lookup(pstate,
																 item->source,
																 strVal(llast(funcname)),
																 fargs,
																 proc_call,
																 fargnames,
																 nargs,
																 argtypes,
																 funcid,
																 rettype,
																 retset,
																 nvargs,
																 vatype,
																 true_typeids,
																 argdefaults,
																 has_udt_type);
				if (result != FUNCDETAIL_NOTFOUND)
				{
					*pfunc = item->source;
					return result;
				}
			}

			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_FUNCTION),
					 errmsg("object type static method \"%s.%s\" does not exist",
							NameListToString(prefix),
							strVal(llast(funcname)))));
		}
	}

	*objoid = InvalidOid;
	return FUNCDETAIL_NOTFOUND;
}

/*
 * LookupObjectTypeConstructor
 *    Attempt to resolve a function call as an object type constructor.
 *    Builds a SELF expression and delegates to the PL/iSQL method lookup.
 */
FuncDetailCode
LookupObjectTypeConstructor(ParseState *pstate,
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
							  bool has_udt_type)
{
	FuncDetailCode		result;
	Oid					targetType;
	ObjectTypeCacheItem *item;
	Oid					constructor_argtypes[FUNC_MAX_ARGS];
	List			   *constructor_fargs;
	Node			   *self_expr;
	ListCell		   *lc;
	int					i;

	*objoid = LookupObjectTypeByNames(funcname, true);
	if (!OidIsValid(*objoid) || proc_call)
		return FUNCDETAIL_NOTFOUND;

	/*
	 * Constructors are executed through the object method infrastructure, so
	 * there is nothing to bind until the body exists and has been compiled
	 * into an object cache entry.
	 */
	if (!OidIsValid(LookupObjectTypeBodyByNames(funcname, true)))
	{
		*objoid = InvalidOid;
		return FUNCDETAIL_NOTFOUND;
	}

	targetType = LookupObjectTypeTypidByNames(funcname, false);
	self_expr = build_object_constructor_self_expr(pstate, targetType, -1);
	constructor_fargs = lcons(self_expr, list_copy(*fargs));

	if (nargs + 1 > FUNC_MAX_ARGS)
		ereport(ERROR,
				(errcode(ERRCODE_TOO_MANY_ARGUMENTS),
				 errmsg_plural("cannot pass more than %d argument to a function",
							   "cannot pass more than %d arguments to a function",
							   FUNC_MAX_ARGS,
							   FUNC_MAX_ARGS)));

	constructor_argtypes[0] = targetType;
	i = 1;
	foreach(lc, *fargs)
		constructor_argtypes[i++] = exprType((Node *) lfirst(lc));

	item = ObjectTypeHandle(*objoid, false);
	if (item == NULL || item->source == NULL ||
		plisql_internal_funcs.object_type_lookup == NULL)
	{
		*objoid = InvalidOid;
		return FUNCDETAIL_NOTFOUND;
	}

	result = plisql_internal_funcs.object_type_lookup(pstate,
													  item->source,
													  strVal(llast(funcname)),
													  &constructor_fargs,
													  false,
													  fargnames,
													  nargs + 1,
													  constructor_argtypes,
													  funcid,
													  rettype,
													  retset,
													  nvargs,
													  vatype,
													  true_typeids,
													  argdefaults,
													  has_udt_type);
	if (result == FUNCDETAIL_NOTFOUND)
	{
		*objoid = InvalidOid;
		return result;
	}

	*fargs = constructor_fargs;
	*rettype = targetType;
	*pfunc = item->source;
	return result;
}

/*
 * lookup_object_member_from_expr
 *    Look up an object method given a receiver expression.
 *    Handles SELF argument injection for member method calls.
 */
static FuncDetailCode
lookup_object_member_from_expr(ParseState *pstate,
							   Node *self_expr,
							   const char *method_name,
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
							   bool has_udt_type,
							   bool self_already_present)
{
	Oid					typoid;
	ObjectTypeCacheItem *item;
	Oid					method_argtypes[FUNC_MAX_ARGS];
	int					method_nargs = nargs;
	List			   *method_fargs = *fargs;

	(void) argtypes;

	typoid = exprType(self_expr);
	if (!OidIsValid(typoid))
		return FUNCDETAIL_NOTFOUND;

	reject_object_constructor_method_target(typoid, method_name);

	item = ObjectTypeHandleByTypid(typoid, true);
	if (item == NULL)
		return FUNCDETAIL_NOTFOUND;

	*objoid = item->key;

	/*
	 * The receiver may already be present when the grammar produced an
	 * explicit object-method call node.  For qualified-name fallback lookups
	 * we prepend SELF here so the PL/iSQL method matcher always sees the
	 * same hidden-first-argument calling convention.
	 */
	if (!self_already_present &&
		method_fargs != NIL &&
		list_length(method_fargs) == nargs + 1 &&
		exprType((Node *) linitial(method_fargs)) == typoid)
		self_already_present = true;

	if (!self_already_present)
	{
		int			i = 1;
		ListCell   *lc;
		List	   *orig_fargs = method_fargs;

		if (nargs + 1 > FUNC_MAX_ARGS)
			ereport(ERROR,
					(errcode(ERRCODE_TOO_MANY_ARGUMENTS),
					 errmsg_plural("cannot pass more than %d argument to a function",
								   "cannot pass more than %d arguments to a function",
								   FUNC_MAX_ARGS,
								   FUNC_MAX_ARGS)));

		method_argtypes[0] = typoid;
		foreach(lc, orig_fargs)
			method_argtypes[i++] = exprType((Node *) lfirst(lc));
		method_fargs = lcons(self_expr, list_copy(orig_fargs));
		method_nargs = nargs + 1;
	}
	else
	{
		int			i = 0;
		ListCell   *lc;

		foreach(lc, method_fargs)
			method_argtypes[i++] = exprType((Node *) lfirst(lc));
		method_nargs = i;
	}

	if (plisql_internal_funcs.object_type_lookup == NULL)
		elog(ERROR, "object type lookup callback is not registered");

	{
		FuncDetailCode result;

		result = plisql_internal_funcs.object_type_lookup(pstate,
														 item->source,
														 method_name,
														 &method_fargs,
														 proc_call,
														 fargnames,
														 method_nargs,
														 method_argtypes,
														 funcid,
														 rettype,
														 retset,
														 nvargs,
														 vatype,
														 true_typeids,
														 argdefaults,
														 has_udt_type);
		if (result == FUNCDETAIL_NOTFOUND)
			return result;

		*fargs = method_fargs;
		*pfunc = item->source;
		return result;
	}
}

/*
 * build_object_constructor_self_expr
 *    Build a NULL-filled composite value as the initial SELF for constructors.
 *    Creates a RowExpr with NULL constants for each attribute.
 */
static Node *
build_object_constructor_self_expr(ParseState *pstate, Oid targetType, int location)
{
	RowExpr	   *rowexpr;
	TupleDesc	tupdesc;
	Node	   *result;
	int			i;

	rowexpr = makeNode(RowExpr);
	rowexpr->args = NIL;
	rowexpr->row_typeid = RECORDOID;
	rowexpr->row_format = COERCE_EXPLICIT_CALL;
	rowexpr->colnames = NIL;
	rowexpr->location = location;

	/*
	 * Constructors are compiled like member routines with an implicit SELF.
	 * Seed that SELF with a NULL-filled composite value of the target type,
	 * then let constructor code assign the final attribute values.
	 */
	tupdesc = lookup_rowtype_tupdesc(targetType, -1);
	for (i = 0; i < tupdesc->natts; i++)
	{
		Form_pg_attribute attr = TupleDescAttr(tupdesc, i);

		if (attr->attisdropped)
			continue;

		rowexpr->args = lappend(rowexpr->args,
								makeNullConst(attr->atttypid,
											  attr->atttypmod,
											  attr->attcollation));
	}
	ReleaseTupleDesc(tupdesc);

	result = coerce_to_target_type(pstate,
								   (Node *) rowexpr,
								   RECORDOID,
								   targetType,
								   -1,
								   COERCION_EXPLICIT,
								   COERCE_EXPLICIT_CALL,
								   location);
	if (result == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_CANNOT_COERCE),
				 errmsg("cannot initialize object type %s for constructor call",
						format_type_be(targetType))));

	return result;
}

static void
reject_object_constructor_method_target(Oid typoid, const char *method_name)
{
	List   *typename;
	Oid		constructor_type;

	if (!OidIsValid(typoid))
		return;

	typename = list_make1(makeString(pstrdup(method_name)));
	constructor_type = LookupObjectTypeTypidByNames(typename, true);
	if (OidIsValid(constructor_type) && constructor_type == typoid)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("constructor \"%s\" cannot be invoked with object method syntax",
						method_name)));
}

/*
 * build_name_prefix
 *    Extract all but the last element from a qualified name list.
 *    Used to separate the type prefix from the method name.
 */
static List *
build_name_prefix(List *funcname)
{
	List	   *prefix = NIL;
	ListCell   *lc;

	foreach(lc, funcname)
	{
		if (lnext(funcname, lc) == NULL)
			break;
		prefix = lappend(prefix, copyObject(lfirst(lc)));
	}

	return prefix;
}
