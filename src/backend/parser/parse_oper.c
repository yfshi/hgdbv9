/*-------------------------------------------------------------------------
 *
 * parse_oper.c
 *		handle operator things for parser
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/parser/parse_oper.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/htup_details.h"
#include "catalog/pg_operator.h"
#include "catalog/pg_type.h"
#include "lib/stringinfo.h"
#include "miscadmin.h"	/* BUG#M0000344 */
#include "nodes/nodeFuncs.h"
#include "nodes/makefuncs.h"
#include "parser/parse_coerce.h"
#include "parser/parse_func.h"
#include "parser/parse_oper.h"
#include "parser/parse_type.h"
#include "utils/builtins.h"
#include "utils/guc.h"			/* ReqID:SRS-GKC-DATATYPE_001 */
#include "utils/inval.h"
#include "utils/lsyscache.h"
#include "utils/ora_compatible.h"	/* ReqID:SRS-GKC-DATATYPE_001 */
#include "utils/syscache.h"
#include "utils/typcache.h"
#include "utils/regproc.h"		/* ReqID:SRS-GKC-DATATYPE_001 */
#include "utils/guc.h"			/* SRS-MISC-COMPATDM-SQL */

/*
 * The lookup key for the operator lookaside hash table.  Unused bits must be
 * zeroes to ensure hashing works consistently --- in particular, oprname
 * must be zero-padded and any unused entries in search_path must be zero.
 *
 * search_path contains the actual search_path with which the entry was
 * derived (minus temp namespace if any), or else the single specified
 * schema OID if we are looking up an explicitly-qualified operator name.
 *
 * search_path has to be fixed-length since the hashtable code insists on
 * fixed-size keys.  If your search path is longer than that, we just punt
 * and don't cache anything.
 */

/* If your search_path is longer than this, sucks to be you ... */
#define MAX_CACHED_PATH_LEN		16

typedef struct OprCacheKey
{
	char		oprname[NAMEDATALEN];
	Oid			left_arg;		/* Left input OID, or 0 if prefix op */
	Oid			right_arg;		/* Right input OID */
	Oid			search_path[MAX_CACHED_PATH_LEN];
} OprCacheKey;

typedef struct OprCacheEntry
{
	/* the hash lookup key MUST BE FIRST */
	OprCacheKey key;

	Oid			opr_oid;		/* OID of the resolved operator */
} OprCacheEntry;

/* Begin - ReqID:SRS-GKC-DATATYPE_001 */
/* Hook to compatible-oracle datatype precedence */
oracle_datatype_precedence_hook_type oracle_datatype_precedence_hook = NULL;
/* End - ReqID:SRS-GKC-DATATYPE_001 */


static Oid	binary_oper_exact(List *opname, Oid arg1, Oid arg2);
static FuncDetailCode oper_select_candidate(int nargs,
											Oid *input_typeids,
											FuncCandidateList candidates,
											Oid *operOid);
static const char *op_signature_string(List *op, char oprkind,
									   Oid arg1, Oid arg2);
static void op_error(ParseState *pstate, List *op, char oprkind,
					 Oid arg1, Oid arg2,
					 FuncDetailCode fdresult, int location);
static bool make_oper_cache_key(ParseState *pstate, OprCacheKey *key,
								List *opname, Oid ltypeId, Oid rtypeId,
								int location);
static Oid	find_oper_cache_entry(OprCacheKey *key);
static void make_oper_cache_entry(OprCacheKey *key, Oid opr_oid);
static void InvalidateOprCacheCallBack(Datum arg, int cacheid, uint32 hashvalue);
static List *needReplaceOpname(List *opname, Oid ltypeId, Oid rtypeId);		/* SRS-MISC-COMPATDM-SQL */

/*
 * LookupOperName
 *		Given a possibly-qualified operator name and exact input datatypes,
 *		look up the operator.
 *
 * Pass oprleft = InvalidOid for a prefix op.
 *
 * If the operator name is not schema-qualified, it is sought in the current
 * namespace search path.
 *
 * If the operator is not found, we return InvalidOid if noError is true,
 * else raise an error.  pstate and location are used only to report the
 * error position; pass NULL/-1 if not available.
 */
Oid
LookupOperName(ParseState *pstate, List *opername, Oid oprleft, Oid oprright,
			   bool noError, int location)
{
	Oid			result;

	result = OpernameGetOprid(opername, oprleft, oprright);
	if (OidIsValid(result))
		return result;

	/* we don't use op_error here because only an exact match is wanted */
	if (!noError)
	{
		char		oprkind;

		if (!OidIsValid(oprleft))
			oprkind = 'l';
		else if (OidIsValid(oprright))
			oprkind = 'b';
		else
		{
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("postfix operators are not supported"),
					 parser_errposition(pstate, location)));
			oprkind = 0;		/* keep compiler quiet */
		}

		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_FUNCTION),
				 errmsg("operator does not exist: %s",
						op_signature_string(opername, oprkind,
											oprleft, oprright)),
				 parser_errposition(pstate, location)));
	}

	return InvalidOid;
}

/*
 * LookupOperWithArgs
 *		Like LookupOperName, but the argument types are specified by
 *		a ObjectWithArgs node.
 */
Oid
LookupOperWithArgs(ObjectWithArgs *oper, bool noError)
{
	TypeName   *oprleft,
			   *oprright;
	Oid			leftoid,
				rightoid;

	Assert(list_length(oper->objargs) == 2);
	oprleft = linitial_node(TypeName, oper->objargs);
	oprright = lsecond_node(TypeName, oper->objargs);

	if (oprleft == NULL)
		leftoid = InvalidOid;
	else
		leftoid = LookupTypeNameOid(NULL, oprleft, noError);

	if (oprright == NULL)
		rightoid = InvalidOid;
	else
		rightoid = LookupTypeNameOid(NULL, oprright, noError);

	return LookupOperName(NULL, oper->objname, leftoid, rightoid,
						  noError, -1);
}

/*
 * get_sort_group_operators - get default sorting/grouping operators for type
 *
 * We fetch the "<", "=", and ">" operators all at once to reduce lookup
 * overhead (knowing that most callers will be interested in at least two).
 * However, a given datatype might have only an "=" operator, if it is
 * hashable but not sortable.  (Other combinations of present and missing
 * operators shouldn't happen, unless the system catalogs are messed up.)
 *
 * If an operator is missing and the corresponding needXX flag is true,
 * throw a standard error message, else return InvalidOid.
 *
 * In addition to the operator OIDs themselves, this function can identify
 * whether the "=" operator is hashable.
 *
 * Callers can pass NULL pointers for any results they don't care to get.
 *
 * Note: the results are guaranteed to be exact or binary-compatible matches,
 * since most callers are not prepared to cope with adding any run-time type
 * coercion steps.
 */
void
get_sort_group_operators(Oid argtype,
						 bool needLT, bool needEQ, bool needGT,
						 Oid *ltOpr, Oid *eqOpr, Oid *gtOpr,
						 bool *isHashable)
{
	TypeCacheEntry *typentry;
	int			cache_flags;
	Oid			lt_opr;
	Oid			eq_opr;
	Oid			gt_opr;
	bool		hashable;

	/* Begin - ReqID:SRS-PLSQL-RECORD */
	/* Records cannot be tested natively for nullity, equality, or inequality */
	if (DB_ORACLE == database_mode &&
		argtype == ORARECORDOID)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				errmsg("unimplemented feature")));
	/* End - ReqID:SRS-PLSQL-RECORD */

	/*
	 * Look up the operators using the type cache.
	 *
	 * Note: the search algorithm used by typcache.c ensures that the results
	 * are consistent, ie all from matching opclasses.
	 */
	if (isHashable != NULL)
		cache_flags = TYPECACHE_LT_OPR | TYPECACHE_EQ_OPR | TYPECACHE_GT_OPR |
			TYPECACHE_HASH_PROC;
	else
		cache_flags = TYPECACHE_LT_OPR | TYPECACHE_EQ_OPR | TYPECACHE_GT_OPR;

	typentry = lookup_type_cache(argtype, cache_flags);
	lt_opr = typentry->lt_opr;
	eq_opr = typentry->eq_opr;
	gt_opr = typentry->gt_opr;
	hashable = OidIsValid(typentry->hash_proc);

	/* Report errors if needed */
	if ((needLT && !OidIsValid(lt_opr)) ||
		(needGT && !OidIsValid(gt_opr)))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_FUNCTION),
				 errmsg("could not identify an ordering operator for type %s",
						format_type_be(argtype)),
				 errhint("Use an explicit ordering operator or modify the query.")));
	if (needEQ && !OidIsValid(eq_opr))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_FUNCTION),
				 errmsg("could not identify an equality operator for type %s",
						format_type_be(argtype))));

	/* Return results as needed */
	if (ltOpr)
		*ltOpr = lt_opr;
	if (eqOpr)
		*eqOpr = eq_opr;
	if (gtOpr)
		*gtOpr = gt_opr;
	if (isHashable)
		*isHashable = hashable;
}


/* given operator tuple, return the operator OID */
Oid
oprid(Operator op)
{
	return ((Form_pg_operator) GETSTRUCT(op))->oid;
}

/* given operator tuple, return the underlying function's OID */
Oid
oprfuncid(Operator op)
{
	Form_pg_operator pgopform = (Form_pg_operator) GETSTRUCT(op);

	return pgopform->oprcode;
}


/* binary_oper_exact()
 * Check for an "exact" match to the specified operand types.
 *
 * If one operand is an unknown literal, assume it should be taken to be
 * the same type as the other operand for this purpose.  Also, consider
 * the possibility that the other operand is a domain type that needs to
 * be reduced to its base type to find an "exact" match.
 */
static Oid
binary_oper_exact(List *opname, Oid arg1, Oid arg2)
{
	Oid			result;
	bool		was_unknown = false;

	if (MYS_PARSER == compatible_db)
	{
		/* Begin - BUG#5123 */
		if (type_is_numeric(arg1) && arg2 == UNKNOWNOID)
		{
			char *schemaname;
			char *opername;

			/* Get operator name */
			/* like operator use text type perform calc*/
			DeconstructQualifiedName(opname, &schemaname, &opername);
			if (strcmp(opername, "~~") == 0 || strcmp(opername, "!~~") == 0)
			{
				arg2 = TEXTOID;
			}
		}
		/* End - BUG#5123 */
	}
	
	/* Unspecified type for one of the arguments? then use the other */
	if ((arg1 == UNKNOWNOID) && (arg2 != InvalidOid))
	{
		arg1 = arg2;
		was_unknown = true;
	}
	else if ((arg2 == UNKNOWNOID) && (arg1 != InvalidOid))
	{
		arg2 = arg1;
		was_unknown = true;
	}

	/* Begin - BUG#Z5201 */
	if (MYS_PARSER == compatible_db &&
		was_unknown && arg1 == FLOAT4OID)
	{
		char	*schemaname;
		char	*opername;

		/* Get operator name */
		DeconstructQualifiedName(opname, &schemaname, &opername);
		if (!strcmp(opername, "<=>") ||
			!strcmp(opername, "+") ||
			!strcmp(opername, "-") ||
			!strcmp(opername, "*") ||
			!strcmp(opername, "/"))
		arg1 = arg2 = FLOAT8OID;
	}
	/* End - BUG#Z5201 */

	/* Begin - OA#1475 */
	if (ORA_PARSER == compatible_db && compatible_ora_bytea_concat_operator &&
		was_unknown && arg1 == BYTEAOID)
	{
		char	*schemaname;
		char	*opername;

		/* Get operator name */
		DeconstructQualifiedName(opname, &schemaname, &opername);
		if (strcmp(opername, "||") == 0)
		{
			arg1 = ORAVARCHARCHAROID;
			arg2 = ORAVARCHARCHAROID;
		}
	}
	/* End - OA#1475 */

	result = OpernameGetOprid(opname, arg1, arg2);
	if (OidIsValid(result))
		return result;

	/* Begin - ReqID:SRS-GKC-DATATYPE_001 */
	if (IsNormalProcessingMode() && oracle_datatype_precedence_hook && ORA_PARSER == compatible_db)
	{	
		bool	rewrite_args = false;
		char	*schemaname;
		char	*opername;
		Oid		result_arg1;
		Oid		result_arg2;

		/* Get operator name */
		DeconstructQualifiedName(opname, &schemaname, &opername);

		rewrite_args = (*oracle_datatype_precedence_hook) (arg1, arg2, opername, &result_arg1, &result_arg2);

		if (rewrite_args)
		{
			arg1 = result_arg1;
			arg2 = result_arg2; 
			
			result = OpernameGetOprid(opname, arg1, arg2);
			if (OidIsValid(result))
				return result;
		}
	}
	/* End - ReqID:SRS-GKC-DATATYPE_001 */

	if (was_unknown)
	{
		/* arg1 and arg2 are the same here, need only look at arg1 */
		Oid			basetype = getBaseType(arg1);

		if (basetype != arg1)
		{
			result = OpernameGetOprid(opname, basetype, basetype);
			if (OidIsValid(result))
				return result;
		}
	}

	return InvalidOid;
}


/* oper_select_candidate()
 *		Given the input argtype array and one or more candidates
 *		for the operator, attempt to resolve the conflict.
 *
 * Returns FUNCDETAIL_NOTFOUND, FUNCDETAIL_MULTIPLE, or FUNCDETAIL_NORMAL.
 * In the success case the Oid of the best candidate is stored in *operOid.
 *
 * Note that the caller has already determined that there is no candidate
 * exactly matching the input argtype(s).  Incompatible candidates are not yet
 * pruned away, however.
 */
static FuncDetailCode
oper_select_candidate(int nargs,
					  Oid *input_typeids,
					  FuncCandidateList candidates,
					  Oid *operOid) /* output argument */
{
	int			ncandidates;

	/*
	 * Delete any candidates that cannot actually accept the given input
	 * types, whether directly or by coercion.
	 */
	ncandidates = func_match_argtypes(nargs, input_typeids,
									  candidates, &candidates);

	/* Done if no candidate or only one candidate survives */
	if (ncandidates == 0)
	{
		*operOid = InvalidOid;
		return FUNCDETAIL_NOTFOUND;
	}
	if (ncandidates == 1)
	{
		*operOid = candidates->oid;
		return FUNCDETAIL_NORMAL;
	}

	/*
	 * Use the same heuristics as for ambiguous functions to resolve the
	 * conflict.
	 */
	candidates = func_select_candidate(nargs, input_typeids, candidates);

	if (candidates)
	{
		*operOid = candidates->oid;
		return FUNCDETAIL_NORMAL;
	}

	*operOid = InvalidOid;
	return FUNCDETAIL_MULTIPLE; /* failed to select a best candidate */
}


/* oper() -- search for a binary operator
 * Given operator name, types of arg1 and arg2, return oper struct.
 *
 * IMPORTANT: the returned operator (if any) is only promised to be
 * coercion-compatible with the input datatypes.  Do not use this if
 * you need an exact- or binary-compatible match; see compatible_oper.
 *
 * If no matching operator found, return NULL if noError is true,
 * raise an error if it is false.  pstate and location are used only to report
 * the error position; pass NULL/-1 if not available.
 *
 * NOTE: on success, the returned object is a syscache entry.  The caller
 * must ReleaseSysCache() the entry when done with it.
 */
Operator
oper(ParseState *pstate, List *opname, Oid ltypeId, Oid rtypeId,
	 bool noError, int location)
{
	Oid			operOid;
	OprCacheKey key;
	bool		key_ok;
	FuncDetailCode fdresult = FUNCDETAIL_NOTFOUND;
	HeapTuple	tup = NULL;

	/* Begin - ReqID:SRS-PLSQL-RECORD */
	/* Records cannot be tested natively for nullity, equality, or inequality */
	if (DB_ORACLE == database_mode &&
		(ltypeId == ORARECORDOID ||
		rtypeId == ORARECORDOID))
	{
		/* Begin - BUG#Z1308 */
		char	   *schemaname;
		char	   *opername;
		/* deconstruct the name list */
		DeconstructQualifiedName(opname, &schemaname, &opername);
		/* End - BUG#Z1308 */

		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				errmsg("wrong number or types of arguments in call to '%s'", opername), /* BUG#Z1308 */
				parser_errposition(pstate, location)));
	}
	/* End - ReqID:SRS-PLSQL-RECORD */

	/*
	 * Try to find the mapping in the lookaside cache.
	 */
	key_ok = make_oper_cache_key(pstate, &key, opname, ltypeId, rtypeId, location);

	if (key_ok)
	{
		operOid = find_oper_cache_entry(&key);
		if (OidIsValid(operOid))
		{
			tup = SearchSysCache1(OPEROID, ObjectIdGetDatum(operOid));
			if (HeapTupleIsValid(tup))
				return (Operator) tup;
		}
	}

	/*
	 * First try for an "exact" match.
	 */
	operOid = binary_oper_exact(opname, ltypeId, rtypeId);
	if (!OidIsValid(operOid))
	{
		/*
		 * Otherwise, search for the most suitable candidate.
		 */
		FuncCandidateList clist;

		/* Get binary operators of given name */
		clist = OpernameGetCandidates(opname, 'b', false);

		/* No operators found? Then fail... */
		if (clist != NULL)
		{
			/*
			 * Unspecified type for one of the arguments? then use the other
			 * (XXX this is probably dead code?)
			 */
			Oid			inputOids[2];

			if (rtypeId == InvalidOid)
				rtypeId = ltypeId;
			else if (ltypeId == InvalidOid)
				ltypeId = rtypeId;
			inputOids[0] = ltypeId;
			inputOids[1] = rtypeId;
			fdresult = oper_select_candidate(2, inputOids, clist, &operOid);
		}
	}

	if (OidIsValid(operOid))
		tup = SearchSysCache1(OPEROID, ObjectIdGetDatum(operOid));

	if (HeapTupleIsValid(tup))
	{
		if (key_ok)
			make_oper_cache_entry(&key, operOid);
	}
	else if (!noError)
		op_error(pstate, opname, 'b', ltypeId, rtypeId, fdresult, location);

	return (Operator) tup;
}

/* compatible_oper()
 *	given an opname and input datatypes, find a compatible binary operator
 *
 *	This is tighter than oper() because it will not return an operator that
 *	requires coercion of the input datatypes (but binary-compatible operators
 *	are accepted).  Otherwise, the semantics are the same.
 */
Operator
compatible_oper(ParseState *pstate, List *op, Oid arg1, Oid arg2,
				bool noError, int location)
{
	Operator	optup;
	Form_pg_operator opform;

	/* oper() will find the best available match */
	optup = oper(pstate, op, arg1, arg2, noError, location);
	if (optup == (Operator) NULL)
		return (Operator) NULL; /* must be noError case */

	/* but is it good enough? */
	opform = (Form_pg_operator) GETSTRUCT(optup);
	if (IsBinaryCoercible(arg1, opform->oprleft) &&
		IsBinaryCoercible(arg2, opform->oprright))
		return optup;

	/* nope... */
	ReleaseSysCache(optup);

	if (!noError)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_FUNCTION),
				 errmsg("operator requires run-time type coercion: %s",
						op_signature_string(op, 'b', arg1, arg2)),
				 parser_errposition(pstate, location)));

	return (Operator) NULL;
}

/* compatible_oper_opid() -- get OID of a binary operator
 *
 * This is a convenience routine that extracts only the operator OID
 * from the result of compatible_oper().  InvalidOid is returned if the
 * lookup fails and noError is true.
 */
Oid
compatible_oper_opid(List *op, Oid arg1, Oid arg2, bool noError)
{
	Operator	optup;
	Oid			result;

	optup = compatible_oper(NULL, op, arg1, arg2, noError, -1);
	if (optup != NULL)
	{
		result = oprid(optup);
		ReleaseSysCache(optup);
		return result;
	}
	return InvalidOid;
}


/* left_oper() -- search for a unary left operator (prefix operator)
 * Given operator name and type of arg, return oper struct.
 *
 * IMPORTANT: the returned operator (if any) is only promised to be
 * coercion-compatible with the input datatype.  Do not use this if
 * you need an exact- or binary-compatible match.
 *
 * If no matching operator found, return NULL if noError is true,
 * raise an error if it is false.  pstate and location are used only to report
 * the error position; pass NULL/-1 if not available.
 *
 * NOTE: on success, the returned object is a syscache entry.  The caller
 * must ReleaseSysCache() the entry when done with it.
 */
Operator
left_oper(ParseState *pstate, List *op, Oid arg, bool noError, int location)
{
	Oid			operOid;
	OprCacheKey key;
	bool		key_ok;
	FuncDetailCode fdresult = FUNCDETAIL_NOTFOUND;
	HeapTuple	tup = NULL;

	/*
	 * Try to find the mapping in the lookaside cache.
	 */
	key_ok = make_oper_cache_key(pstate, &key, op, InvalidOid, arg, location);

	if (key_ok)
	{
		operOid = find_oper_cache_entry(&key);
		if (OidIsValid(operOid))
		{
			tup = SearchSysCache1(OPEROID, ObjectIdGetDatum(operOid));
			if (HeapTupleIsValid(tup))
				return (Operator) tup;
		}
	}

	/*
	 * First try for an "exact" match.
	 */
	operOid = OpernameGetOprid(op, InvalidOid, arg);
	if (!OidIsValid(operOid))
	{
		/*
		 * Otherwise, search for the most suitable candidate.
		 */
		FuncCandidateList clist;

		/* Get prefix operators of given name */
		clist = OpernameGetCandidates(op, 'l', false);

		/* No operators found? Then fail... */
		if (clist != NULL)
		{
			/*
			 * The returned list has args in the form (0, oprright). Move the
			 * useful data into args[0] to keep oper_select_candidate simple.
			 * XXX we are assuming here that we may scribble on the list!
			 */
			FuncCandidateList clisti;

			for (clisti = clist; clisti != NULL; clisti = clisti->next)
			{
				clisti->args[0] = clisti->args[1];
			}

			/*
			 * We must run oper_select_candidate even if only one candidate,
			 * otherwise we may falsely return a non-type-compatible operator.
			 */
			fdresult = oper_select_candidate(1, &arg, clist, &operOid);
		}
	}

	if (OidIsValid(operOid))
		tup = SearchSysCache1(OPEROID, ObjectIdGetDatum(operOid));

	if (HeapTupleIsValid(tup))
	{
		if (key_ok)
			make_oper_cache_entry(&key, operOid);
	}
	else if (!noError)
		op_error(pstate, op, 'l', InvalidOid, arg, fdresult, location);

	return (Operator) tup;
}

/*
 * op_signature_string
 *		Build a string representing an operator name, including arg type(s).
 *		The result is something like "integer + integer".
 *
 * This is typically used in the construction of operator-not-found error
 * messages.
 */
static const char *
op_signature_string(List *op, char oprkind, Oid arg1, Oid arg2)
{
	StringInfoData argbuf;

	initStringInfo(&argbuf);

	if (oprkind != 'l')
		appendStringInfo(&argbuf, "%s ", format_type_be(arg1));

	appendStringInfoString(&argbuf, NameListToString(op));

	appendStringInfo(&argbuf, " %s", format_type_be(arg2));

	return argbuf.data;			/* return palloc'd string buffer */
}

/*
 * op_error - utility routine to complain about an unresolvable operator
 */
static void
op_error(ParseState *pstate, List *op, char oprkind,
		 Oid arg1, Oid arg2,
		 FuncDetailCode fdresult, int location)
{
	if (fdresult == FUNCDETAIL_MULTIPLE)
		ereport(ERROR,
				(errcode(ERRCODE_AMBIGUOUS_FUNCTION),
				 errmsg("operator is not unique: %s",
						op_signature_string(op, oprkind, arg1, arg2)),
				 errhint("Could not choose a best candidate operator. "
						 "You might need to add explicit type casts."),
				 parser_errposition(pstate, location)));
	else
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_FUNCTION),
				 errmsg("operator does not exist: %s",
						op_signature_string(op, oprkind, arg1, arg2)),
				 (!arg1 || !arg2) ?
				 errhint("No operator matches the given name and argument type. "
						 "You might need to add an explicit type cast.") :
				 errhint("No operator matches the given name and argument types. "
						 "You might need to add explicit type casts."),
				 parser_errposition(pstate, location)));
}

/*
 * make_op()
 *		Operator expression construction.
 *
 * Transform operator expression ensuring type compatibility.
 * This is where some type conversion happens.
 *
 * last_srf should be a copy of pstate->p_last_srf from just before we
 * started transforming the operator's arguments; this is used for nested-SRF
 * detection.  If the caller will throw an error anyway for a set-returning
 * expression, it's okay to cheat and just pass pstate->p_last_srf.
 */
Expr *
make_op(ParseState *pstate, List *opname, Node *ltree, Node *rtree,
		Node *last_srf, bool has_outer_join, int location, int opkind)	/* ReqID:SRS-SQL-OUTERJOIN(+) */	/* SRS-MISC-COMPATDM-SQL */
{
	Oid			ltypeId,
				rtypeId;
	Operator	tup;
	Form_pg_operator opform;
	Oid			actual_arg_types[2];
	Oid			declared_arg_types[2];
	int			nargs;
	List	   *args;
	Oid			rettype;
	OpExpr	   *result;

	/* Check it's not a postfix operator */
	if (rtree == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("postfix operators are not supported")));

	/* Select the operator */
	if (ltree == NULL)
	{
		/* prefix operator */
		rtypeId = exprType(rtree);
		ltypeId = InvalidOid;
		tup = left_oper(pstate, opname, rtypeId, false, location);
	}
	else
	{
		/* Begin - BUG#M000411 */
		bool has_change_type = false;
		/* End - BUG#M0000411 */
		/* Begin - BUG#Z0001452 */
		Oid save_origltypeId = InvalidOid;
		Oid save_origrtypeId = InvalidOid;
		/* End - BUG#Z00001452 */

		/* otherwise, binary operator */
		ltypeId = exprType(ltree);
		rtypeId = exprType(rtree);

		/** 
		 * Begin - ReqID:SRS-MYSQL-ENUMTYPE
		 * If both left and right expressions are enum type, we trans it to ANYENUM;
		 * If one of left and right expreessions is enum type, and the other is UNKNOWN,
		 * we trans the UNKNOWN to text.
		 */
		if (compatible_db == MYS_PARSER)
		{
			const char *op_name = strVal(linitial(opname));

			if (type_is_enum(ltypeId) && type_is_enum(rtypeId))
			{
				save_origltypeId = ltypeId;
				save_origrtypeId = rtypeId;
				has_change_type = true;
				ltypeId = ANYENUMOID;
				rtypeId = ANYENUMOID;
			}

			/**
			 * Begin - SRS-MYSQL-HEX-LITERAL
			 * When PIPES_AS_CONCAT is on, cast hex to text.
			 */
			if (!strcmp(op_name, "||") && has_sql_mode(MODE_PIPES_AS_CONCAT))
			{
				if (ltypeId == IVY_HEXOID)
				{
					ltree = coerce_type(pstate,
										ltree, IVY_HEXOID,
										TEXTOID, -1,
										COERCION_EXPLICIT, COERCE_EXPLICIT_CALL,
										exprLocation(ltree));

					ltypeId = TEXTOID;
				}

				if (rtypeId == IVY_HEXOID)
				{
					rtree = coerce_type(pstate,
										rtree, IVY_HEXOID,
										TEXTOID, -1,
										COERCION_EXPLICIT, COERCE_EXPLICIT_CALL,
										exprLocation(rtree));
							
					rtypeId = TEXTOID;
				}
			}
			/* End - SRS-MYSQL-HEX-LITERAL */
		}
		/* End - ReqID:SRS-MYSQL-ENUMTYPE */

		if (ORA_PARSER == compatible_db)
		{
			/* 
			 * Begin - SRS-MISC-COMPATDM-SQL
			 * replace the comparison opname as needed when enable_blankspace_compare is off,
			 * this ignore triled spaces in some querys,except(distinct,group by,set operation).
			 */
			if(!enable_blankspace_compare && (opkind == AEXPR_OP || opkind == AEXPR_IN || opkind == ROWCOMPARISONOP) )
			{
				List *opname_rewrite = needReplaceOpname(opname, ltypeId, rtypeId);
				if (opname_rewrite)
					opname = opname_rewrite;
			}
			/* End - SRS-MISC-COMPATDM-SQL */
			/* Begin - BUG#M000411 */
			/*
			 * there if const integer operater const integer
			 * we convert it to const number operater const number
			 * so that it like oracle, because oracle integer is number(38,0)
			 */
			if (enable_compatible_const_integer_operator &&
				ltypeId == INT4OID && nodeTag(ltree) == T_Const &&
				rtypeId == INT4OID && nodeTag(rtree) == T_Const)
			{
				/* Begin - BUG#Z0001452 */
				save_origltypeId = ltypeId;
				save_origrtypeId = rtypeId;
				/* End - BUG#Z00001452 */
				ltypeId = NUMBEROID;
				rtypeId = NUMBEROID;
				has_change_type = true;
			}
			/* Begin - BUG#Z0001452 */
			/*
			 * At least one of the parties is of numeric type,
			 * we use number / number
			 */
			else if (enable_compatible_numeric_division_result &&
					((ltypeId == INT2OID ||
					  ltypeId == INT4OID ||
					  ltypeId == INT8OID ||
					  ltypeId == UNKNOWNOID) &&
					 (rtypeId == INT2OID ||
					  rtypeId == INT4OID ||
					  rtypeId == INT8OID ||
					  rtypeId == UNKNOWNOID)) &&
					list_length(opname) == 1 &&
					strcmp(strVal(linitial(opname)), "/") == 0)
			{
				save_origltypeId = ltypeId;
				save_origrtypeId = rtypeId;
				has_change_type = true;
				ltypeId = NUMBEROID;
				rtypeId = NUMBEROID;
			}
			/* End - BUG#Z00001452 */
			/* End - BUG#M0000411 */
		}
		
		tup = oper(pstate, opname, ltypeId, rtypeId, false, location);
		/* Begin - BUG#M000411 */
		if (has_change_type)
		{
			/* Begin - BUG#Z0001452 */
			ltypeId = save_origltypeId;
			rtypeId = save_origrtypeId;
			/* End - BUG#Z00001452 */
		}
		/* End - BUG#M0000411 */
	}

	opform = (Form_pg_operator) GETSTRUCT(tup);

	/* Check it's not a shell */
	if (!RegProcedureIsValid(opform->oprcode))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_FUNCTION),
				 errmsg("operator is only a shell: %s",
						op_signature_string(opname,
											opform->oprkind,
											opform->oprleft,
											opform->oprright)),
				 parser_errposition(pstate, location)));

	/* Do typecasting and build the expression tree */
	if (ltree == NULL)
	{
		/* prefix operator */
		args = list_make1(rtree);
		actual_arg_types[0] = rtypeId;
		declared_arg_types[0] = opform->oprright;
		nargs = 1;
	}
	else
	{
		/* otherwise, binary operator */
		args = list_make2(ltree, rtree);
		actual_arg_types[0] = ltypeId;
		actual_arg_types[1] = rtypeId;
		declared_arg_types[0] = opform->oprleft;
		declared_arg_types[1] = opform->oprright;
		nargs = 2;
	}

	/*
	 * enforce consistency with polymorphic argument and return types,
	 * possibly adjusting return type or declared_arg_types (which will be
	 * used as the cast destination by make_fn_arguments)
	 */
	rettype = enforce_generic_type_consistency(actual_arg_types,
											   declared_arg_types,
											   nargs,
											   opform->oprresult,
											   false);

	/* perform the necessary typecasting of arguments */
	make_fn_arguments(pstate, args, actual_arg_types, declared_arg_types);

	/* and build the expression node */
	result = makeNode(OpExpr);
	result->opno = oprid(tup);
	result->opfuncid = opform->oprcode;
	result->opresulttype = rettype;
	result->opretset = get_func_retset(opform->oprcode);
	/* opcollid and inputcollid will be set by parse_collate.c */
	result->args = args;
	result->has_outer_join = has_outer_join;	/* ReqID:SRS-SQL-OUTERJOIN(+) */
	result->location = location;

	/* if it returns a set, check that's OK */
	if (result->opretset)
	{
		check_srf_call_placement(pstate, last_srf, location);
		/* ... and remember it for error checks at higher levels */
		pstate->p_last_srf = (Node *) result;
	}

	ReleaseSysCache(tup);

	/* Begin - ReqID:SRS-MYSQL-SET
	 * if both sides of the operator are user defined variable,
	 * determine the target type for conversion and store it.
	 */
	if (ltree != NULL && nodeTag(ltree) == T_VarRef)
	{
		VarRef *lref;

		lref = (VarRef *)ltree;
		lref->rettype = exprType((Node *)linitial(result->args));
		linitial(result->args) = lref;
	}
	if (rtree != NULL && nodeTag(rtree) == T_VarRef)
	{
		VarRef *rref;

		rref = (VarRef *)rtree;
		rref->rettype = exprType((Node *)llast(result->args));
		llast(result->args) = rref;
	}
	/* End - ReqID:SRS-MYSQL-SET */

	return (Expr *) result;
}

/*
 * make_scalar_array_op()
 *		Build expression tree for "scalar op ANY/ALL (array)" construct.
 */
Expr *
make_scalar_array_op(ParseState *pstate, List *opname,
					 bool useOr,
					 Node *ltree, Node *rtree,
					 int location)
{
	Oid			ltypeId,
				rtypeId,
				atypeId,
				res_atypeId;
	Operator	tup;
	Form_pg_operator opform;
	Oid			actual_arg_types[2];
	Oid			declared_arg_types[2];
	List	   *args;
	Oid			rettype;
	ScalarArrayOpExpr *result;

	ltypeId = exprType(ltree);
	atypeId = exprType(rtree);

	/*
	 * The right-hand input of the operator will be the element type of the
	 * array.  However, if we currently have just an untyped literal on the
	 * right, stay with that and hope we can resolve the operator.
	 */
	if (atypeId == UNKNOWNOID)
		rtypeId = UNKNOWNOID;
	else
	{
		rtypeId = get_base_element_type(atypeId);
		if (!OidIsValid(rtypeId))
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("op ANY/ALL (array) requires array on right side"),
					 parser_errposition(pstate, location)));
	}
	/* 
	 * Begin - SRS-MISC-COMPATDM-SQL,
	 * if we are in make_scalar_array_op,it means we are dealing with ANY|ALL|IN a_expr,
	 * replace the opname as needed 
	 */
	if(!enable_blankspace_compare && ORA_PARSER == compatible_db)
	{
		List * opname_rewrite = needReplaceOpname(opname,ltypeId,rtypeId);
		if(opname_rewrite)
			opname = opname_rewrite;
	}
	/* End - SRS-MISC-COMPATDM-SQL */
	/* Now resolve the operator */
	tup = oper(pstate, opname, ltypeId, rtypeId, false, location);
	opform = (Form_pg_operator) GETSTRUCT(tup);

	/* Check it's not a shell */
	if (!RegProcedureIsValid(opform->oprcode))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_FUNCTION),
				 errmsg("operator is only a shell: %s",
						op_signature_string(opname,
											opform->oprkind,
											opform->oprleft,
											opform->oprright)),
				 parser_errposition(pstate, location)));

	args = list_make2(ltree, rtree);
	actual_arg_types[0] = ltypeId;
	actual_arg_types[1] = rtypeId;
	declared_arg_types[0] = opform->oprleft;
	declared_arg_types[1] = opform->oprright;

	/*
	 * enforce consistency with polymorphic argument and return types,
	 * possibly adjusting return type or declared_arg_types (which will be
	 * used as the cast destination by make_fn_arguments)
	 */
	rettype = enforce_generic_type_consistency(actual_arg_types,
											   declared_arg_types,
											   2,
											   opform->oprresult,
											   false);

	/*
	 * Check that operator result is boolean
	 */
	if (rettype != BOOLOID)
		ereport(ERROR,
				(errcode(ERRCODE_WRONG_OBJECT_TYPE),
				 errmsg("op ANY/ALL (array) requires operator to yield boolean"),
				 parser_errposition(pstate, location)));
	if (get_func_retset(opform->oprcode))
		ereport(ERROR,
				(errcode(ERRCODE_WRONG_OBJECT_TYPE),
				 errmsg("op ANY/ALL (array) requires operator not to return a set"),
				 parser_errposition(pstate, location)));

	/*
	 * Now switch back to the array type on the right, arranging for any
	 * needed cast to be applied.  Beware of polymorphic operators here;
	 * enforce_generic_type_consistency may or may not have replaced a
	 * polymorphic type with a real one.
	 */
	if (IsPolymorphicType(declared_arg_types[1]))
	{
		/* assume the actual array type is OK */
		res_atypeId = atypeId;
	}
	else
	{
		res_atypeId = get_array_type(declared_arg_types[1]);
		if (!OidIsValid(res_atypeId))
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_OBJECT),
					 errmsg("could not find array type for data type %s",
							format_type_be(declared_arg_types[1])),
					 parser_errposition(pstate, location)));
	}
	actual_arg_types[1] = atypeId;
	declared_arg_types[1] = res_atypeId;

	/* perform the necessary typecasting of arguments */
	make_fn_arguments(pstate, args, actual_arg_types, declared_arg_types);

	/* and build the expression node */
	result = makeNode(ScalarArrayOpExpr);
	result->opno = oprid(tup);
	result->opfuncid = opform->oprcode;
	result->hashfuncid = InvalidOid;
	result->useOr = useOr;
	/* inputcollid will be set by parse_collate.c */
	result->args = args;
	result->location = location;

	ReleaseSysCache(tup);

	return (Expr *) result;
}


/*
 * Lookaside cache to speed operator lookup.  Possibly this should be in
 * a separate module under utils/cache/ ?
 *
 * The idea here is that the mapping from operator name and given argument
 * types is constant for a given search path (or single specified schema OID)
 * so long as the contents of pg_operator and pg_cast don't change.  And that
 * mapping is pretty expensive to compute, especially for ambiguous operators;
 * this is mainly because there are a *lot* of instances of popular operator
 * names such as "=", and we have to check each one to see which is the
 * best match.  So once we have identified the correct mapping, we save it
 * in a cache that need only be flushed on pg_operator or pg_cast change.
 * (pg_cast must be considered because changes in the set of implicit casts
 * affect the set of applicable operators for any given input datatype.)
 *
 * XXX in principle, ALTER TABLE ... INHERIT could affect the mapping as
 * well, but we disregard that since there's no convenient way to find out
 * about it, and it seems a pretty far-fetched corner-case anyway.
 *
 * Note: at some point it might be worth doing a similar cache for function
 * lookups.  However, the potential gain is a lot less since (a) function
 * names are generally not overloaded as heavily as operator names, and
 * (b) we'd have to flush on pg_proc updates, which are probably a good
 * deal more common than pg_operator updates.
 */

/* The operator cache hashtable */
static HTAB *OprCacheHash = NULL;


/*
 * make_oper_cache_key
 *		Fill the lookup key struct given operator name and arg types.
 *
 * Returns true if successful, false if the search_path overflowed
 * (hence no caching is possible).
 *
 * pstate/location are used only to report the error position; pass NULL/-1
 * if not available.
 */
static bool
make_oper_cache_key(ParseState *pstate, OprCacheKey *key, List *opname,
					Oid ltypeId, Oid rtypeId, int location)
{
	char	   *schemaname;
	char	   *opername;

	/* deconstruct the name list */
	DeconstructQualifiedName(opname, &schemaname, &opername);

	/* ensure zero-fill for stable hashing */
	MemSet(key, 0, sizeof(OprCacheKey));

	/* save operator name and input types into key */
	strlcpy(key->oprname, opername, NAMEDATALEN);
	key->left_arg = ltypeId;
	key->right_arg = rtypeId;

	if (schemaname)
	{
		ParseCallbackState pcbstate;

		/* search only in exact schema given */
		setup_parser_errposition_callback(&pcbstate, pstate, location);
		key->search_path[0] = LookupExplicitNamespace(schemaname, false);
		cancel_parser_errposition_callback(&pcbstate);
	}
	else
	{
		/* get the active search path */
		if (fetch_search_path_array(key->search_path,
									MAX_CACHED_PATH_LEN) > MAX_CACHED_PATH_LEN)
			return false;		/* oops, didn't fit */
	}

	return true;
}

/*
 * find_oper_cache_entry
 *
 * Look for a cache entry matching the given key.  If found, return the
 * contained operator OID, else return InvalidOid.
 */
static Oid
find_oper_cache_entry(OprCacheKey *key)
{
	OprCacheEntry *oprentry;

	if (OprCacheHash == NULL)
	{
		/* First time through: initialize the hash table */
		HASHCTL		ctl;

		ctl.keysize = sizeof(OprCacheKey);
		ctl.entrysize = sizeof(OprCacheEntry);
		OprCacheHash = hash_create("Operator lookup cache", 256,
								   &ctl, HASH_ELEM | HASH_BLOBS);

		/* Arrange to flush cache on pg_operator and pg_cast changes */
		CacheRegisterSyscacheCallback(OPERNAMENSP,
									  InvalidateOprCacheCallBack,
									  (Datum) 0);
		CacheRegisterSyscacheCallback(CASTSOURCETARGET,
									  InvalidateOprCacheCallBack,
									  (Datum) 0);
	}

	/* Look for an existing entry */
	oprentry = (OprCacheEntry *) hash_search(OprCacheHash,
											 (void *) key,
											 HASH_FIND, NULL);
	if (oprentry == NULL)
		return InvalidOid;

	return oprentry->opr_oid;
}

/*
 * make_oper_cache_entry
 *
 * Insert a cache entry for the given key.
 */
static void
make_oper_cache_entry(OprCacheKey *key, Oid opr_oid)
{
	OprCacheEntry *oprentry;

	Assert(OprCacheHash != NULL);

	oprentry = (OprCacheEntry *) hash_search(OprCacheHash,
											 (void *) key,
											 HASH_ENTER, NULL);
	oprentry->opr_oid = opr_oid;
}

/*
 * Callback for pg_operator and pg_cast inval events
 */
static void
InvalidateOprCacheCallBack(Datum arg, int cacheid, uint32 hashvalue)
{
	HASH_SEQ_STATUS status;
	OprCacheEntry *hentry;

	Assert(OprCacheHash != NULL);

	/* Currently we just flush all entries; hard to be smarter ... */
	hash_seq_init(&status, OprCacheHash);

	while ((hentry = (OprCacheEntry *) hash_seq_search(&status)) != NULL)
	{
		if (hash_search(OprCacheHash,
						(void *) &hentry->key,
						HASH_REMOVE, NULL) == NULL)
			elog(ERROR, "hash table corrupted");
	}
}

/* Begin - SRS-MISC-COMPATDM-SQL */
static List *
needReplaceOpname(List *opname, Oid ltypeId, Oid rtypeId)
{
	/* replace opname as needed. */
	char	*schemaname;
	char	*opername = NULL;
	char	*rewritename = palloc(sizeof(char)*NAMEDATALEN);
	/* We only deal with binary operator(=,<,>,<=,>=,!=) */
	
	/* Expect these situations */
	if(!OidIsValid(ltypeId) || !OidIsValid(rtypeId))
		return NIL;
	if(!ISDMCHARTYPE(ltypeId) || !ISDMCHARTYPE(rtypeId))
		return NIL;
	if(ltypeId == UNKNOWNOID && rtypeId == UNKNOWNOID)
		return NIL;

	/* Only deal with (=,<,>,<=,>=,!=) */
	DeconstructQualifiedName(opname, &schemaname, &opername);
	if(!opername || !ISCOMPAREOPNAME(opername))
		return NIL;
	
	snprintf(rewritename,NAMEDATALEN,"#%s",opername);
	return list_make1(makeString(rewritename));
}
/* End - SRS-MISC-COMPATDM-SQL */
