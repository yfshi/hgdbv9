/*-------------------------------------------------------------------------
 *
 * File: RedactionPolicyAnalyze.c
 *
 * Abstract: support data redacation policy analyze.
 *
 * Copyright:
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *			/contrib/hg_redaction/RedactionPolicyAnalyze.c
 *
 * ReqID:SRS-SECURITY-DATADESENSITIZATION
 *
 *-------------------------------------------------------------------------
 */
#include "hg_redaction.h"


/*---- Function declarations ----*/
static void process_redaction(ParseState *pstate, Query *query, bool toptlistflg, bool dealtartlist);
static void select_PostParseAnalyze(ParseState *pstate, Query *query, bool toptlistflg, bool dealtartlist);
static bool handle_masking(List* targetList, ParseState *pstate,
								List* rtable, Node* utilityNode, bool toptlistflg);
static bool parser_target_entry(ParseState *pstate, TargetEntry **old_tle,
									   List* rtable, bool can_mask, bool toptlistflg);
static bool handle_masking_node(ParseState *pstate, Expr **src_expr,
									   List* rtable, bool can_mask, bool toptlistflg, bool eqflg);
static int get_var_value(const List *rtable, Var *var,
									VarParserValue *varvalue);
static void reset_node_location();
static Node* create_predefined_function(const char* funcname, int funcid, int rettype, 
												  Node* arg, int funccollid);
static Node* create_relabel_type(Node* func, int resulttype, int location);
static Node *redaction_data_function(ParseState *pstate, Var *var, 
											 Oid funcid, char *funcname);
static Node *redaction_data_all_function(ParseState *pstate, Var *var, 
												   Oid funcid, char *funcname);
static Node *create_bool_node(ParseState *pstate, int value, int location);
static bool mask_expr_node(ParseState *pstate, Expr **expr, List* rtable, bool can_mask, bool toptlistflg, bool eqflg);
static bool hg_handle_redaction_data(ParseState *pstate, Query *query, JumbleState *jstate, bool eqflg);
static bool redaction_check(char *polname, char *colname, int16 scope, int16 execption, bool toptlistflg, bool eqflg);
static void redaction_whexception(ParseState *pstate, Query *query);
static bool	redaction_option_cmp(char *tbvalue, char *currval);
static bool redaction_op_isequal(OpExpr *opexpr);
static char *default_time_str(Var *var);

PG_FUNCTION_INFO_V1(redaction_all_name);

int		node_location = 0;

/*
 * reset_node_location
 *	no value return.
 *
 * NOTE: reset a node location.
 */
static
void reset_node_location()
{
	node_location = 0;
}

/*
 * hg_handle_redaction_parse_analyze
 *		no value return.
 *
 * NOTE: data redaction main entry.
 */
void
hg_handle_redaction_parse_analyze(ParseState *pstate, Query *query, JumbleState *jstate)
{
	if (hg_prev_redaction_parse_analyze)
		hg_prev_redaction_parse_analyze(pstate, query, jstate);

	(void)hg_handle_redaction_data(pstate, query, jstate, true);
}

/*
 * check_redaction_tb
 *		return a bool value.
 *
 * NOTE: jude redaction table exists.
 */
static bool
check_redaction_tb(void)
{
	char	*rdpolicy = "hg_redaction_policy";
	char	*rdpolicycol = "hg_redaction_column";
	Oid		rdpolicyschemaoid = InvalidOid;

	/*get the oid of hg_security schema */
	rdpolicyschemaoid = SECURITY_SCHEMA_NAMESPACE;

	/* if hg_security schema exists, the next step is get the oid of 
	 * hg_redaction_policy and hg_redaction_column, otherwise report an error.
	 */
	if (OidIsValid(rdpolicyschemaoid) &&
		SearchSysCacheExists2(RELNAMENSP, PointerGetDatum(rdpolicy), ObjectIdGetDatum(rdpolicyschemaoid)) &&
		SearchSysCacheExists2(RELNAMENSP, PointerGetDatum(rdpolicycol), ObjectIdGetDatum(rdpolicyschemaoid)))
	{
		return true;
	}

	return false;
}

/*
 * hg_handle_redaction_data
 *		return a bool value.
 *
 * Returns value is a bool value, if the data redaction policy is off,
 * return false, return true if the data redaction is on.
 *
 * NOTE: data redaction according to different types.
 */
static bool
hg_handle_redaction_data(ParseState *pstate, Query *query,JumbleState *jstate, bool toptlistflg)
{
	/*
	 * if the guc paramater hg_redaction_data is off,
	 * does not data redaction.
	 */
	if (!hg_redaction_data)
		return false;

	if (!check_redaction_tb())
		return false;

	if (IS_SYSSSO(GetUserId()))
		return false;

	/*
	 * Perform relevant operations according to different
	 * command type.
	 */
	if (query->commandType == CMD_SELECT)
	{
		reset_node_location();
		process_redaction(pstate, query, true, false);
	}

	if (query->commandType == CMD_SELECT || query->commandType == CMD_UPDATE ||
		query->commandType == CMD_DELETE || query->commandType == CMD_INSERT)
	{
		if (query->rtable != NIL)
		{
			ListCell *lc = NULL;
			/* there is deal with return list for top query */
			if (query->returningList != NIL)
			{
				handle_masking(query->returningList, pstate,
					query->rtable, query->utilityStmt, false);
			}

			/* there is deal with target list for top query */
			if (query->commandType != CMD_SELECT && query->targetList != NIL)
			{
				handle_masking(query->targetList, pstate,
								query->rtable, query->utilityStmt, toptlistflg);
			}

			/* Deal with data redaction for subquery */
			foreach (lc, query->rtable)
			{
				RangeTblEntry *rte = (RangeTblEntry *)lfirst(lc);
				if (rte->rtekind == RTE_SUBQUERY && rte->subquery != NULL)
				{
					handle_masking(rte->subquery->targetList, pstate,
						rte->subquery->rtable, rte->subquery->utilityStmt, false);
				}
			}
		}
	}

	return true;
}

/*
 * get_function_behavious
 *		return a int value.
 *
 * The returns value is a integer, it represents the data
 * redaction funtcion type.
 *
 * NOTE:Get the type of data redaction function by the
 * name of the function.
 */
int
get_function_behavious(const char *func_name)
{
	int func_type = M_UNKNOWN;
	int i = 0;

	for (i = 0; masking_funcs_infos[i].func != NULL; i++)
	{
		if (pg_strcasecmp(masking_funcs_infos[i].func, func_name) == 0)
		{
			func_type = masking_funcs_infos[i].type;
		}
	}

	return func_type;
}

/*
 * get_var_value
 *		return a int value.
 *
 * The returns value is a integer, it represents the type
 * of RangeTableEntry.
 *
 * NOTE:In TargetList, each TargetEntry's varno points to rtable.
 * So, the RangeTableEntry corresponding to each varno needs
 * to be got first.Additionally, parse full column name for
 * every (var-rte) pair.
 */
static int
get_var_value(const List *rtable, Var *var, VarParserValue *varvalue)
{
	RangeTblEntry  *rte;

	if ((var && var->varno >= 1) && 
		(list_length(rtable) && (list_length(rtable) >= var->varno)))
	{
		rte = (RangeTblEntry *) list_nth(rtable, var->varno - 1);
	}
	else
	{
		/*
		 * if rte is not found return NO_FOUND_RTE_OR_COLUMN,
		 * no data redaction will be performed.
		 */
		return NO_FOUND_RTE_OR_COLUMN;
	}

	/*
	 * Determine whether the current table is a system
	 * table, if it is a system table, no data redaction
	 * will be performed.
	 */
	if (!OidIsValid(rte->relid) || IsCatalogRelationOid(rte->relid))
		return CATALOG_RELID;

	/* the kind of current RangeTableEntry */
	switch (rte->rtekind)
	{
		case RTE_RELATION:
		{
			Relation	tbl_rel;
			char		relkind;

			relkind = get_rel_relkind(rte->relid);
			varvalue->table_oid = rte->relid;
			tbl_rel = relation_open(varvalue->table_oid, AccessShareLock);
			if (RelationIsValid(tbl_rel))
			{
				if (tbl_rel->rd_rules &&
					(relkind == RELKIND_VIEW || relkind == RELKIND_MATVIEW))
				{
					relation_close(tbl_rel, AccessShareLock);
					return VIEW_OR_MATVIEW_RELID;
				}
				else
				{
					/* find the exact column names */
					if ((var->varattno >= 1) &&
						(rte && rte->eref && list_length(rte->eref->colnames) &&
						 (list_length(rte->eref->colnames) >= var->varattno)))
					{
						snprintf(varvalue->col_name, sizeof(varvalue->col_name), "%s",
								 strVal(list_nth(rte->eref->colnames, var->varattno - 1 )));
					}
					else
					{
						/*
						 * if var is not found return NO_FOUND_RTE_OR_COLUMN,
						 * no data redaction will be performed.
						 */
						return NO_FOUND_RTE_OR_COLUMN;
					}
				}
			}
			relation_close(tbl_rel, AccessShareLock);
		}
			break;

		default:
			break;
		
	}
	return 1;
}

/*
 * create_bool_node
 *		return a node.
 *
 * The returns value is node, the node of the bool
 * value result.
 *
 * NOTE: A new node is a type conversion node, the
 * processing of bool value.
 */
static Node
*create_bool_node(ParseState *pstate, int value, int location)
{
	int col_type = BOOLOID;
	bool make_cast = true;
	Node * const_node = (Node *) make_const(pstate, makeString("false"), location);
	
	if (make_cast)
	{
		const_node = coerce_type(pstate, const_node, ((Const*)const_node)->consttype, col_type, -1,
								 COERCION_IMPLICIT, COERCE_IMPLICIT_CAST, -1);
	}
	return const_node;
}

/*
 * create_integer_node
 *		return a node.
 *
 * The returns value is node, the node of the integer
 * value result.
 *
 * NOTE: A new node is a type conversion node, the
 * processing of integer value.
 */
static Node
*create_integer_node(ParseState *pstate, int value, int location, int col_type, bool make_cast)
{
	Node * const_node;

	const_node = (Node *) make_const(pstate, makeInteger(value), location);

	if (make_cast)
	{
		if (col_type == INT2OID)
			const_node = coerce_type(pstate, const_node, ((Const*)const_node)->consttype, col_type, -1,
								 COERCION_EXPLICIT, COERCE_EXPLICIT_CALL, -1);
		else
			const_node = coerce_type(pstate, const_node, ((Const*)const_node)->consttype, col_type, -1,
								 COERCION_IMPLICIT, COERCE_IMPLICIT_CAST, -1);
	}
	return const_node;
}

/*
 * create_string_node
 *		return a node.
 *
 * The returns value is node, the node of the text
 * value result.
 *
 * NOTE: A new node is a type conversion node, the
 * processing of text value.
 */
static Node 
*create_string_node(ParseState *pstate, const char* letter, int location, Oid varoid)
{
	Oid col_type = TEXTOID;
	Node * const_node;

	if (OidIsValid(varoid))
		col_type = varoid;

	const_node = (Node *) make_const(pstate, makeString((char*)letter), location);
	const_node = coerce_type(pstate, const_node, ((Const*)const_node)->consttype, col_type, -1,
							 COERCION_IMPLICIT, COERCE_IMPLICIT_CAST, -1);
	return const_node;
}

/*
 * get_function_id
 *		return a bool value.
 *
 * The returns value is true, it means that the
 * function is found, otherwise it is not found.
 *
 * NOTE: Find function by function name.
 */
static bool
get_function_id(int funcrettype, const char* funcname, Oid varoid, Oid *funcid, Oid *rettype, Oid *allParams, int allCount)
{
	Oid				schemaid;
	HeapTuple		proctup;
	oidvector	   *parameterTypes;

	parameterTypes = buildoidvector(allParams, allCount);

	if (!pg_strcasecmp(funcname, "redact_all_name"))
		schemaid = SECURITY_SCHEMA_NAMESPACE;
	else
		schemaid = PG_CATALOG_NAMESPACE;

	proctup = SearchSysCache3(PROCNAMEARGSNSP,
					PointerGetDatum(funcname),
					PointerGetDatum(parameterTypes),
					ObjectIdGetDatum(schemaid));

	if (HeapTupleIsValid(proctup))
	{
		Form_pg_proc procform = (Form_pg_proc) GETSTRUCT(proctup);

		(*funcid) = procform->oid;
		(*rettype) = procform->prorettype;

		ReleaseSysCache(proctup);

		return true;
	}

	return false;
}

/*
 * create_repeat_function
 *		return a node.
 *
 * The returns value is node. the node repersents
 * a function node, which is the length of data
 * change after data redaction.
 *
 * NOTE: Construct a new function node to represent
 * the form displayed after data redaction.
 */
static Node
*create_repeat_function(ParseState *pstate, const char *letter, Node *arg)
{
	Var	   *var = (Var*)arg;
	Oid		funcid = InvalidOid;
	Oid		rettype = InvalidOid;
	Oid		allParams[2];

	/*
	 * get the repeat function oid and return type,
	 * create a new node, the function if this nide
	 * is to determin the format of the data after
	 * data redaction.
	 */
	allParams[0] = TEXTOID;
	allParams[1] = INT4OID;
	if (get_function_id(TEXTOID, "repeat", var->vartype, &funcid, &rettype, allParams, 2))
	{
		Node	   *const_node = create_string_node(pstate, letter, var->location, InvalidOid);
		Node	   *repeat_func = create_predefined_function("repeat", funcid,
															 rettype, const_node, 100);
		FuncExpr   *funcexpr = (FuncExpr*)repeat_func;

		allParams[0] = TEXTOID;
		if (get_function_id(INT4OID, "length", var->vartype, &funcid, &rettype, allParams, 1))
		{
			Node *length_func = create_predefined_function("length", funcid, rettype, arg, 0);
			funcexpr->args = lappend(funcexpr->args, (Node*)length_func);
		}
		return repeat_func;
	}
	return arg;
}

/*
 * create_relabel_type
 *		return a node.
 *
 * The returns value is not null, which represents the data
 * redaction performed
 *
 * NOTE: Constructs a new node from a function
 * expression node and a return type.
 */
static Node* 
create_relabel_type(Node* func, int resulttype, int location)
{
	CoercionForm	relabelformat = COERCE_EXPLICIT_CAST;
	RelabelType	   *typeexpr;

	if (func == NULL)
		return NULL;

	/*
	 * create a RelabelType expression,
	 * and fill the value.
	 */
	typeexpr = makeNode(RelabelType);

	if (!typeexpr)
		return NULL;

	typeexpr->arg = (Expr*)func;
	typeexpr->resulttype = resulttype;
	typeexpr->resulttypmod = -1;

	/* OID of collation */
	if (resulttype == NAMEOID)
		typeexpr->resultcollid = 950;
	else
		typeexpr->resultcollid = 100;
	typeexpr->relabelformat = relabelformat;
	typeexpr->location = location;
	return (Node*)typeexpr;
}

/*
 * create_predefined_function
 *		return a node.
 *
 * The returns a node, represents the formation of
 * a new data redaction node.
 *
 * NOTE: A new node is formed by the oid of the
 * function, the function name, and the parameter
 * of the function, the new node is a data redaction
 * node.
 */
static Node* 
create_predefined_function(const char* funcname, int funcid, int rettype, 
										Node* arg, int funccollid)
{
	CoercionForm funcformat = COERCE_EXPLICIT_CALL;

	/*
	 * create a FuncExpr expression,
	 * and fill the value.
	 */
	FuncExpr   *funcexpr = makeNode(FuncExpr);
	funcexpr->funcid = funcid;
	funcexpr->funcresulttype = rettype;
	funcexpr->funcretset = false;
	funcexpr->funcvariadic = false;
	funcexpr->funcformat = funcformat;

	if (IsA(arg, Var))
	{
		Var* var = (Var*)arg;
		var->location += node_location;
		funcexpr->location = var->location;
		var->location += strlen(funcname) + 1;
		node_location += (strlen(funcname) + 2);
	}
	else if(IsA(arg, Const))
	{
		Const* const_arg = (Const*)arg;
		const_arg->location += node_location;
		funcexpr->location = const_arg->location;
		const_arg->location += strlen(funcname) + 1;
		node_location += (strlen(funcname) + 2);
	}
	else if(IsA(arg, FuncExpr))
	{
		FuncExpr* fe = (FuncExpr*)arg;
		fe->location += node_location;
		funcexpr->location = fe->location;
		fe->location += strlen(funcname) + 1;
		node_location += (strlen(funcname) + 2);
	}

	funcexpr->args = lappend(funcexpr->args, arg);
	funcexpr->funccollid = funccollid;

	/* OID of collation that function should use */
	funcexpr->inputcollid = 100;

	return (Node*)funcexpr;
}

/*
 * redaction_data_all_function
 *		return a node.
 *
 * The returns value is node, which represents the data
 * redaction node.
 *
 * NOTE: A new node is formed by the oid of the
 * function, the function name, and the parameter
 * of the function.
 */
static Node *
redaction_data_all_function(ParseState *pstate, Var *var, 
							 Oid funcid, char *funcname)
{
	char time_str[BUFFSIZE] = {0};
	Node *maskall_func_node = NULL;

	/*
	 * It is mainly divided into four parts,
	 * bool type, time type, string type,
	 * and number type.
	 */
	switch (var->vartype)
	{
		case BOOLOID:
		{
			/*
			 * Constructs an implicit conversion node
			 * for type bool.
			 */
			Node *const_int_node = create_bool_node(pstate, 0, var->location);
			if (const_int_node != NULL)
				maskall_func_node = const_int_node;
		}
		break;

		case TIMEOID:
		case TIMETZOID:
		case INTERVALOID:
		case TIMESTAMPOID:
		case TIMESTAMPTZOID:
		{
			Node *const_node = NULL;

			/* create a string node after data redaction */
			const_node = create_string_node(pstate, default_time_str(var), var->location, var->vartype);
			if (const_node != NULL)
			{
				maskall_func_node = const_node;
			}
		}
		break;

		case TEXTOID:
		case BPCHAROID:
		case VARCHAROID:
		{
			/* the number of characters of data redaction */
			maskall_func_node = create_repeat_function(pstate, "x", (Node*)var);
			if (IsA(maskall_func_node, FuncExpr))
			{
				if (((FuncExpr*)maskall_func_node)->funcresulttype != var->vartype)
				{
					Expr* castfun = (Expr*)create_relabel_type(maskall_func_node, var->vartype, var->location);
					if (castfun != NULL)
						maskall_func_node = (Node*)castfun;
				}
			}
		}
		break;

		case NAMEOID:
		{
			Oid		funcid = InvalidOid;
			Oid		rettype = InvalidOid;
			Oid		allParams[2];

			allParams[0] = NAMEOID;
			if (get_function_id(NAMEOID, "redact_all_name", var->vartype, &funcid, &rettype, allParams, 1))
			{
				maskall_func_node = create_predefined_function("redact_all_name", funcid,
															 rettype, (Node *)var, 100);
			}
			if (!maskall_func_node)
				maskall_func_node = (Node *)var;
		}
		break;

		case INT8OID:
		case INT4OID:
		case INT2OID:
		case NUMERICOID:
		case FLOAT4OID: /* real */
		case FLOAT8OID:
		{
			Node *const_int_node = NULL;
			bool flag = (var->vartype != CASHOID);

			/* create a integer node after data redaction */
			const_int_node = create_integer_node(pstate, 0, var->location,
												 var->vartype, flag);
			if (const_int_node != NULL)
				maskall_func_node = const_int_node;
		}
		break;

		case CASHOID:
		{
			Node *const_int_node = NULL;
			char *cashfuncname = "redact_cash";

			if (!pg_strcasecmp(funcname, "redact_all"))
			{
				funcname = pstrdup(cashfuncname);
				funcid = get_procid_from_procname(cashfuncname);
			}
			const_int_node = create_predefined_function(funcname, funcid, var->vartype, (Node*)var, 100);
			if (const_int_node != NULL)
				maskall_func_node = const_int_node;
		}
		break;

		default: /* wrong column type */
		{
			char *typnamespace = NULL;
			char *typname = NULL;

			if (OidIsValid(var->vartype))
			{
				typnamespace = get_typenamespace(var->vartype);
				typname = format_type_be(var->vartype);
			}

			if ((typname != NULL && pg_strcasecmp(typname, "seclabel") == 0) &&
				(typnamespace != NULL && pg_strcasecmp(typnamespace, "hg_security") == 0))
			{
				Node *const_node = NULL;

				/* Display value after seclabel type data redaction */
				snprintf(time_str, sizeof(time_str), "s*****");
				const_node = create_string_node(pstate, time_str, var->location, TEXTOID);
				maskall_func_node = const_node;
			}
			else
			{
				ereport(WARNING,
						(errcode(ERRCODE_WRONG_OBJECT_TYPE),
						 errmsg("Unsupported type of column will not be masked by policy.")));
			}

			break;
		}
	}
	return maskall_func_node;
}

/*
 * redaction_data_function
 *		return a node.
 *
 * The returns value is not null, which represents the data
 * redaction performed
 *
 * NOTE: A new node is formed by the oid of the
 * function, the function name, and the parameter
 * of the function.
 */
static Node 
*redaction_data_function(ParseState *pstate, Var *var, Oid funcid, char *funcname)
{
	bool make_cast = false;
	Node *credit_func_node = NULL;

	if (var->vartype == BPCHAROID || var->vartype == VARCHAROID)
	{
		make_cast = true;
	}

	switch (var->vartype)
	{
		/*
		 * if var type is char(n) or varchar,
		 * need to change the type to text.
		 */
		case BPCHAROID:
		case VARCHAROID:
		case TEXTOID:
		{
			credit_func_node = create_predefined_function(funcname, funcid, 
															TEXTOID, (Node*)var, 100);

			if (make_cast)
			{
				/* Construct a type conversion node,
				 * the return type is the column type.
				 */
				Expr* castfun = (Expr*)create_relabel_type(credit_func_node, 
															var->vartype, var->location);
				if (castfun != NULL)
					credit_func_node = (Node*)castfun;
			}
		break;
		}
	}
	return credit_func_node;
}

/*
 * mask_node_by_behavious
 *		return a node.
 *
 * The returns value is node, which represents the data
 * redaction performed.
 *
 * NOTE: A new node is formed by the oid of the
 * function, the function name, the redaction
 * data function type, and the parameter of the
 * function.
 */
static Node *mask_node_by_behavious(ParseState *pstate, Var *var, Oid funcid, char *funcname, int functype)
{
	Node *masked_node = NULL;
	switch (functype)
	{
		case REDACT_BANKCARD:
		case REDACT_EMAILNAME:
		case REDACT_EMAILFULL:
		case REDACT_DIGITS:
		case REDACT_SHUFFLE:
		case REDACT_RANDOM:
		case REDACT_IDCARD:
		case REDACT_NAME:
		case REDACT_PHONE:
		case REDACT_CASH:
		{
			/*
			 * If the type if var is text, bpchar and varchar are
			 * processed by function redaction_data_function,
			 * otherwise use function redaction_data_all_function
			 * to process.
			 */
			if ((masked_node = redaction_data_function(pstate, var, funcid, funcname)) == NULL)
			{
				masked_node = redaction_data_all_function(pstate, var, funcid, funcname);
			}
		}
			break;
		case REDACT_ALL:
			masked_node = redaction_data_all_function(pstate, var, funcid, funcname);
			break;
		default:
			masked_node = redaction_data_all_function(pstate, var, funcid, funcname);
			break;

	}

	return masked_node;
}

/*
 * redaction_option_cmp
 *	   return a bool value.
 *
 * NOTE: the function main Function is compare ip.
 */
static bool
redaction_option_cmp(char *tbvalue, char *currval)
{
	char	   *copytbvalue;
	List	   *tbvaluelist = NIL;

	if (currval && !strcmp(tbvalue, currval))
		return true;
	else
	{
		copytbvalue = pstrdup(tbvalue);
		if (currval && SplitIdentifierString(copytbvalue, ',', &tbvaluelist))
		{
			ListCell   *namecell;

			foreach(namecell, tbvaluelist)
			{
				char *eachval = NULL;

				eachval = (char *)lfirst(namecell);
				if (eachval && !strcmp(eachval, currval))
					return true;
				else if (!strcmp("[local]", currval))
				{
					if (eachval && !strcasecmp(eachval, "127.0.0.1"))
						return true;
				}
			}
		}
		else if (currval && !strcmp("[local]", currval))
		{
			if (!strcasecmp(tbvalue, "127.0.0.1"))
				return true;
		}
	}

	return false;
}

/*
 * getoptions_list
 *		return a bool value.
 *
 * if the data redaction policy information obtained
 * througth the oid if this data redaction policy is
 * consistent with the information of the current
 * exectutor,return true, otherwise return false.
 *
 * NOTE: the main function of the function is to check
 * the for clause of the data redaction policy
 */
static bool
getoptions_list(Oid poloid)
{
	HeapTuple		policy_tuple;
	bool			toptions_list_isnull;
	Datum			datum = (Datum)0;
	List		   *options;
	ListCell	   *cell;
	List		   *col_list = NULL;
	char		   *value = NULL;
	char		   *sessionuser_name = NULL;
	char		   *user_name= NULL;
	Relation		policyrel;
	SysScanDesc		sscan_p;
	ScanKeyData		skey2;

	/* get the name of the user and current session user */
	user_name = GetUserNameFromId(GetUserId(),false);
	sessionuser_name = GetUserNameFromId(GetSessionUserId(),false);

	/*
	 * Get the value of the column poloptions from table
	 * hg_redaction_policy through the oid of the policy.
	 */
	policyrel = table_open(get_policytb_oid(), AccessShareLock);
	ScanKeyInit(&skey2,
				Anum_hg_redaction_policy_polid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(poloid));
	
	sscan_p = systable_beginscan(policyrel, get_policytbindex_oid(NULL, hg_redaciton_policy_oid_index),
						true, NULL, 1, &skey2);
	policy_tuple = systable_getnext(sscan_p);

	if (policy_tuple)
	{
		datum = heap_getattr(policy_tuple, Anum_hg_redaction_policy_poloptions, 
								RelationGetDescr(policyrel), &toptions_list_isnull);
	}

	table_close(policyrel, AccessShareLock);
	systable_endscan(sscan_p);

	/*
	 * parser the value of the column poloptions from table
	 * hg_redaction_policy, determine whether the current
	 * environment requires data redaction.
	 * DEFELEM_SET is "!="
	 */
	if (!toptions_list_isnull && datum)
	{
		options = redatcion_untransformRelOptions(datum);

		foreach(cell, options)
		{
			DefElem    *def = (DefElem    *)lfirst(cell);

			value = NULL;
			/*
			 * def->defname cannot be null, if value is null,
			 * start the next loop.
			 */
			if (def->defname != NULL)
			{
				col_list = (List *) def->arg;

				if (col_list->type == T_String)
					value = strVal(col_list);
			}

			if (!value)
				continue;

			/* Start matchinging each option */
			if ((pg_strcasecmp(def->defname,"user")) == 0)
			{
				if (def->defaction == DEFELEM_SET)
				{
					if (!redaction_option_cmp(value, user_name))
						return true;
				}
				else
				{
					if (redaction_option_cmp(value, user_name))
						return true;
				}
			}
			else if ((pg_strcasecmp(def->defname,"ip")) == 0)
			{
				struct Port *tmpport = MyProcPort;

				if (tmpport && tmpport->remote_host)
				{
					if (def->defaction == DEFELEM_SET)
					{
						if (!redaction_option_cmp(value, tmpport->remote_host))
							return true;
					}
					else 
					{
						if (redaction_option_cmp(value, tmpport->remote_host))
							return true;
					}
				}
			}
			else if((pg_strcasecmp(def->defname,"application")) == 0)
			{
				if (application_name)
				{
					if (def->defaction == DEFELEM_SET)
					{
						if (!redaction_option_cmp(value, application_name))
							return true;
					}
					else
					{
						if (redaction_option_cmp(value, application_name))
							return true;
					}
				}
			}
			else if((pg_strcasecmp(def->defname,"session_user")) == 0)
			{
				if (def->defaction == DEFELEM_SET)
				{
					if (!redaction_option_cmp(value, sessionuser_name))
						return true;
				}
				else
				{
					if (redaction_option_cmp(value, sessionuser_name))
						return true;
				}
			}
			else if((pg_strcasecmp(def->defname,value)) == 0)
			{
				if (def->defaction == DEFELEM_SET)
					return false;
				else
					return true;
			}
			else if((pg_strcasecmp(def->defname,"true")) == 0)
			{
				if (value && !strcmp(def->defname, value))
				{
					if (def->defaction == DEFELEM_SET)
						return false;
					else
						return true;
				}
				else
					return false;
			}
			else
			{
				return false;
			}
		}
	}
	else
	{
		return true;
	}

	return false;
}

/*
 * handle_masking_node
 *		return a bool value.
 *
 * The returns value is true, which represents the data
 * redaction performed
 *
 * NOTE: Find the data redaction policy, obtain relevant
 * information, perform data redaction on the column, and
 * return the wrapped new node.
 */
static bool
handle_masking_node(ParseState *pstate, Expr **src_expr,
						   List* rtable, bool can_mask, bool toptlistflg, bool eqflg)
{
	Var				   *var;
	VarParserValue	   *varvalue;
	bool				is_masking = false;
	int					ret;
	Relation			attrel;
	ScanKeyData			skey[2];
	SysScanDesc			scan;
	HeapTuple			contup;
	Oid					func_oid;
	char			   *func_name;
	int					func_type;
	Node			   *masked_node = NULL;
	Oid					policy_id;
	Relation			policyrel;
	SysScanDesc			sscan_p = NULL;
	HeapTuple			policy_tuple;
	ScanKeyData			skey2;
	bool				isEnable;
	bool				isnull;
	Datum				poliddatum;
	Datum				polenabledatum;
	Datum				funcoiddatum;
	Datum				funcnamedatum;
	Datum				polnamedatum;
	Datum				colnamedatum;
	Datum				scopedatum;
	Datum				execpdatum;
	char			   *pol_name;
	char			   *col_name;
	int16				scopenum;
	int16				excptionnum;
	bool				rdflg;

	/* is the expression is null, return false */
	if (*src_expr == NULL)
		return false;

	var = (Var *)(*src_expr);

	varvalue = (VarParserValue *)palloc(sizeof(VarParserValue));

	/*
	 * Check where the column comes form, get the column name
	 * and the oid of the table where the column is located,
	 * if it is a system table, there will be no data redaction.
	 */
	ret = get_var_value(rtable, var, varvalue);

	/* is the var in system table, return false */
	if (ret == CATALOG_RELID || ret == NO_FOUND_RTE_OR_COLUMN)
		return false;

	/*
	 * Through attname in hg_redaction_column finds the oid
	 * of policy.
	 */
	attrel = table_open(get_policycoltb_oid(), AccessShareLock);

	ScanKeyInit(&skey[0],
				Anum_hg_redaction_column_tabrelid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(varvalue->table_oid));

	ScanKeyInit(&skey[1],
				Anum_hg_redaction_column_attname,
				BTEqualStrategyNumber, F_NAMEEQ,
				CStringGetDatum(varvalue->col_name));

	scan = systable_beginscan(attrel, get_policytbindex_oid(NULL, hg_redaciton_column_tabrelid_attname_index), true,
								 NULL, 2, skey);

	/* There can be at most one matching row */
	if ((contup = systable_getnext(scan)) == NULL)
	{
		systable_endscan(scan);
		table_close(attrel, AccessShareLock);
		return false;
	}

	/* Find the oid of policy. */
	poliddatum = heap_getattr(contup, Anum_hg_redaction_column_polid, 
								RelationGetDescr(attrel), &isnull);
	policy_id = DatumGetObjectId(poliddatum);

	policyrel = table_open(get_policytb_oid(), AccessShareLock);
	/*
	 * if the data redaction policy existing, need to check
	 * the current data redaction policy enable or disable,
	 * if the current policy is off, return false.
	 */
	if (OidIsValid(policy_id))
	{
		ScanKeyInit(&skey2,
					Anum_hg_redaction_policy_polid,
					BTEqualStrategyNumber, F_OIDEQ,
					ObjectIdGetDatum(policy_id));

		sscan_p = systable_beginscan(policyrel,
									 get_policytbindex_oid(NULL, hg_redaciton_policy_oid_index),
 									 true, NULL, 1, &skey2);
		policy_tuple = systable_getnext(sscan_p);

		if (policy_tuple == NULL)
		{
			systable_endscan(scan);
			table_close(attrel, AccessShareLock);
			systable_endscan(sscan_p);
			table_close(policyrel, AccessShareLock);
			return false;
		}

		/* get the status of current data redaction policy,
		 * if disable return false.
		 */
		polenabledatum = heap_getattr(policy_tuple, Anum_hg_redaction_policy_polpermissive,
									RelationGetDescr(policyrel), &isnull);
		isEnable = DatumGetBool(polenabledatum);
		if (!isEnable)
		{
			systable_endscan(scan);
			table_close(attrel, AccessShareLock);
			systable_endscan(sscan_p);
			table_close(policyrel, AccessShareLock);
			return false;
		}
	}

	/*
	 * Find the oid and name of the function used
	 * by the policy on the the current column.
	 */
	funcoiddatum = heap_getattr(contup, Anum_hg_redaction_column_procid, 
								RelationGetDescr(attrel), &isnull);
	func_oid = DatumGetObjectId(funcoiddatum);

	funcnamedatum = heap_getattr(contup, Anum_hg_redaction_column_procname,
								RelationGetDescr(attrel), &isnull);
	func_name = NameStr(*(DatumGetName(funcnamedatum)));

	/* there check redaction or not */
	polnamedatum = heap_getattr(contup, Anum_hg_redaction_column_polname,
								RelationGetDescr(attrel), &isnull);
	pol_name = NameStr(*(DatumGetName(polnamedatum)));

	colnamedatum = heap_getattr(contup, Anum_hg_redaction_column_attname,
								RelationGetDescr(attrel), &isnull);
	col_name = NameStr(*(DatumGetName(colnamedatum)));

	scopedatum = heap_getattr(contup, Anum_hg_redaction_column_polscope, 
									RelationGetDescr(attrel), &isnull);
	scopenum = DatumGetInt16(scopedatum);

	execpdatum = heap_getattr(contup, Anum_hg_redaction_column_polexception, 
									RelationGetDescr(attrel), &isnull);
	excptionnum = DatumGetInt16(execpdatum);

	rdflg = redaction_check(pol_name, col_name, scopenum,
							excptionnum, toptlistflg, eqflg);

	/*
	 * get the type of data redaction function,
	 * if the function is not a predefined function,
	 * return false.
	 */
	func_type = get_function_behavious(func_name);

	if (func_type == M_UNKNOWN)
	{
		systable_endscan(sscan_p);
		table_close(policyrel, AccessShareLock);
		systable_endscan(scan);
		table_close(attrel, AccessShareLock);
		return false;
	}

	/*
	 * Here is a judgment on the conditions of the
	 * policy, whether data redaction is required.
	 */
	if (!(getoptions_list(policy_id)))
	{
		systable_endscan(sscan_p);
		table_close(policyrel, AccessShareLock);
		systable_endscan(scan);
		table_close(attrel, AccessShareLock);

		return false;
	}

	/*
	 * Real data redaction operation.
	 */
	 if (rdflg)
		masked_node = mask_node_by_behavious(pstate, var, func_oid, func_name, func_type);

	/*
	 * if have data redaction, modify the original expression.
	 */
	if (masked_node != NULL)
	{
		*src_expr = (Expr *)masked_node;
		is_masking = true;
	}

	systable_endscan(sscan_p);
	table_close(policyrel, AccessShareLock);
	systable_endscan(scan);
	table_close(attrel, AccessShareLock);
	
	return is_masking;
}

/*
 * mask_func
 *	   return a bool value.
 *
 * NOTE: The input parameters of the function are the columns of
 * the table, there is a data redaction policy on the column, and
 * the column needs to be processed.
 */
static bool
mask_func(ParseState *pstate, Expr **expr, List* rtable, bool can_mask, bool toptlistflg, bool eqflg)
{
	bool is_masking = false;

	if (expr == NULL)
	{
		return false;
	}

	if (nodeTag(*expr) == T_FuncExpr)
	{
		FuncExpr *func = (FuncExpr *)(*expr);

		if (func->args != NULL)
		{
			ListCell* temp = NULL;

			foreach(temp, func->args)
			{
				Node *item = (Node*)lfirst(temp);
				bool expr_masked = mask_expr_node(pstate, (Expr **)&item, rtable, 
													can_mask, toptlistflg, false);
				is_masking = is_masking || expr_masked;

				if (is_masking)
					lfirst(temp) = item;
			}
		}
	}

	return is_masking;
}

/*
 * mask_sublink
 *		return a bool value.
 *
 * The returns value is true, indicating that a node
 * has performed data redaction.
 *
 * NOTE: The main processing is the target list of the
 * subquery.
 */
static bool
mask_sublink(ParseState *pstate, Expr **expr, List* rtable, bool can_mask, bool toptlistflg, bool eqflg)
{
	SubLink *sublink;
	Query *query;
	ListCell* temp = NULL;
	bool is_masking = false;

	/* if the node is null, return null */
	if (*expr == NULL)
		return false;

	sublink= (SubLink *) (*expr);
	query = (Query *) sublink->subselect;

	/* Process each TargetEntry sequentially */
	foreach (temp, query->targetList)
	{
		TargetEntry *old_tle = (TargetEntry *) lfirst(temp);
		is_masking = parser_target_entry(pstate, &old_tle, query->rtable, can_mask, toptlistflg) || is_masking;
	}

	return is_masking;
}

/*
 * mask_list_parameters
 *		return a bool value.
 *
 * The returns value is true, indicating that a node
 * has performed data redaction.
 *
 * NOTE: The main processing is the target list of the
 * subquery.
 */
static bool
mask_list_parameters(List **params, ParseState *pstate, bool *is_masking, List* rtable, bool can_mask, bool eqflg, bool toptlistflg)
{
	List* masked_list = NIL;
	ListCell   *lc = NULL;

	/* Process each node sequentially */
	foreach(lc, (*params))
	{
		Node *item = (Node *) lfirst(lc);

		switch (nodeTag(item)) 
		{
			case T_Aggref:
			{
				Aggref* agg = (Aggref *) item;

				if (agg && agg->args != NIL && list_length(agg->args) > 0)
				{
					mask_list_parameters(&(agg->args), pstate, is_masking, rtable, can_mask, false, false);
				}
				break;
			}
			case T_OpExpr:
			{
				OpExpr* opexpr = (OpExpr*)item;
				bool	eqflg = false;

				eqflg = redaction_op_isequal(opexpr);

				if (opexpr && opexpr->args != NIL && list_length(opexpr->args) > 0)
				{
					mask_list_parameters(&(opexpr->args), pstate, is_masking, rtable, can_mask, eqflg, toptlistflg);
				}
				break;
			}
			case T_Var:
			case T_RelabelType:
			case T_FuncExpr:
			{
				bool expr_masked = mask_expr_node(pstate, (Expr **)&item, rtable, can_mask, toptlistflg, eqflg);
				*is_masking = expr_masked || *is_masking;
				break;
			}
			case T_TargetEntry:
			{
				TargetEntry** target_entry = (TargetEntry **) &item;
				bool expr_masked = parser_target_entry(pstate, target_entry, rtable, can_mask, toptlistflg);

				*is_masking = expr_masked || *is_masking;
				break;
			}
			default:
				break;
		}
		masked_list = lappend(masked_list, item);
	}
	if (*is_masking)
		(*params) = masked_list;
	return *is_masking;
}

/*
 * mask_expr_node
 *		return a bool value.
 *
 * The returns value is true, which represents the data
 * redaction performed
 *
 * NOTE: According to diffent types, wrap the columns
 * that require data redaction into new nodes.
 */
static bool
mask_expr_node(ParseState *pstate, Expr **expr, List* rtable, bool can_mask, bool toptlistflg, bool eqflg)
{
	/* if the node is null, return */
	if (*expr == NULL)
		return false;

	/*
	 * Different processing is performed according
	 * to different node types.
	 */
	switch (nodeTag(*expr))
	{
		case T_SubLink:
			return mask_sublink(pstate, expr, rtable, can_mask, false, false);

		case T_FuncExpr:
			return mask_func(pstate, expr, rtable, can_mask, toptlistflg, eqflg);

		case T_Var:
			return handle_masking_node(pstate, expr, rtable, true, toptlistflg, eqflg);
		case T_RelabelType:
		{
			RelabelType *relabel = (RelabelType *) (*expr);

			if (relabel->arg != NULL)
			{
				return mask_expr_node(pstate, (Expr **)&(relabel->arg), rtable, can_mask, toptlistflg, eqflg);
			}
			break;
		}
		case T_CoerceViaIO:
		{
			CoerceViaIO *coerce = (CoerceViaIO *) (*expr);

			if (coerce->arg != NULL)
			{
				return mask_expr_node(pstate, (Expr **)&(coerce->arg),  rtable, false, toptlistflg, eqflg);
			}
			break;
		}
		case T_Aggref:
		{
			Aggref *agg = (Aggref *) (*expr);

			if (agg->args != NIL && list_length(agg->args) > 0)
			{
				bool is_masking = false;

				mask_list_parameters(&(agg->args), pstate, &is_masking, rtable, true, false, toptlistflg);
				return is_masking;
			}
			break;
		}
		case T_OpExpr:
		{
			OpExpr *opexpr = (OpExpr *) (*expr);
			bool	eqflg = false;

			eqflg = redaction_op_isequal(opexpr);

			if (opexpr->args != NIL && list_length(opexpr->args) > 0)
			{
				bool is_masking = false;

				mask_list_parameters(&(opexpr->args), pstate, &is_masking, rtable, true, eqflg, toptlistflg);
				return is_masking;
			}
			break;
		}
		default:
			break;
	}
	return false;
}

/*
 * parser_target_entry
 *		return a bool value.
 *
 * The returns value is true, which represents the data
 * redaction performed
 *
 * NOTE: Perform data redaction on columns with data redacion policy
 */
static bool
parser_target_entry(ParseState *pstate, TargetEntry **old_tle,
						   List* rtable, bool can_mask, bool toptlistflg)
{
	Node *src_expr = (Node *)(*old_tle)->expr;
	bool is_masking = false;

	switch (nodeTag(src_expr))
	{
		case T_SubLink:
		{
			SubLink		*sublink = (SubLink *) src_expr;
			Query		*query = (Query *) sublink->subselect;
			ListCell	*temp = NULL;

			/* Recursion process every target column of the sublink */
			foreach (temp, query->targetList)
			{
				TargetEntry *old_tle = (TargetEntry *) lfirst(temp);
				parser_target_entry(pstate, &old_tle, query->rtable, can_mask, false);
			}
		}
			break;
		case T_Var:
		{
			/*
			 * Process target column, if there is a data redaction
			 * policy on the column.
			 */
			is_masking = handle_masking_node(pstate, &((*old_tle)->expr),
											 rtable, can_mask, toptlistflg, false);
			if (is_masking)
			{
				(*old_tle)->resorigtbl = 0;
				(*old_tle)->resorigcol = 0;
			}
		}
			break;

		case T_Aggref:
		case T_OpExpr:
		case T_RelabelType:
		case T_FuncExpr:
		case T_CoerceViaIO:
		{
			/*
			 * Handle the expression of this node, if there is
			 * a data redaction policy on the column, the column
			 * as a parameter of this expression.
			 */
			if (mask_expr_node(pstate, &((*old_tle)->expr), rtable, can_mask, toptlistflg, false))
			{
				(*old_tle)->resorigtbl = 0;
				(*old_tle)->resorigcol = 0;
				is_masking = true;
			}
		}
			break;
		default:
			break;
	}
	return is_masking;
}

/*
 * handle_masking
 *		return a bool value.
 *
 * The returns value has no meaning, just for code logic.
 *
 * NOTE: Process each target node in turn.
 */
static bool
handle_masking(List* targetList, ParseState *pstate,
					List* rtable, Node* utilityNode, bool toptlistflg)
{
	ListCell* temp = NULL;

	if (targetList == NIL)
		return false;

	if (pstate->p_subselect)
		toptlistflg = false;

	foreach(temp, targetList)
	{
		TargetEntry *old_tle = (TargetEntry *) lfirst(temp);

		/*
		 * Shuffle masking columns can only select directly
		 * with out other operations.
		 */
		parser_target_entry(pstate, &old_tle, rtable, true, toptlistflg);
	}

	return false;
}

/*
 * select_PostParseAnalyze
 *		no return value.
 *
 * NOTE: start data redaction on the target list
 * at each level.
 */
static void
select_PostParseAnalyze(ParseState *pstate, Query *query, bool toptlistflg, bool dealtartlist)
{
	List *targetList = NIL;

	if (query == NULL)
		return;

	/* Only the target list is process here. */
	if (query->targetList != NIL) 
		targetList = query->targetList;

	/* data redaction */
	if (dealtartlist)
		handle_masking(targetList, pstate, query->rtable, query->utilityStmt, toptlistflg);

	redaction_whexception(pstate, query);
	
	return;
}

/*
 * redaction_whexception
 *		no return value.
 *
 * NOTE: Handle the exception in the where clause.
 */
static void
redaction_whexception(ParseState *pstate, Query *query)
{
	if (query && query->jointree)
	{
		FromExpr	*fexpr;

		/*
		 * Handles simple equals expressions in the where clause
		 */
		fexpr = (FromExpr *)query->jointree;
		if ( fexpr && fexpr->quals && (nodeTag(fexpr->quals) == T_OpExpr))
		{
			mask_expr_node(pstate, (Expr **)&fexpr->quals, query->rtable, true, false, false);
		}
		/*
		 * Handle the exception of the bool expression in the where clause
		 */
		else if (fexpr && fexpr->quals && (nodeTag(fexpr->quals) == T_BoolExpr))
		{
			BoolExpr	*bexpr = (BoolExpr *)fexpr->quals;
			bool is_masking = false;

			/* deal with exctpion */
			mask_list_parameters(&(bexpr->args), pstate, &is_masking,
									query->rtable, true, false, false);
		}
		/*
		 * Handle the exception of the bool expression in the where clause.
		 *
		 * It will not processed for the time being, because the design of
		 * the design if the sublink cannot accurately locate the range if
		 * the range of the table to be searched. select * from tb where col
		 * in (select col from tb);
		 */
		else if (fexpr && fexpr->quals && (nodeTag(fexpr->quals) == T_SubLink))
		{
			SubLink	   *sublink = (SubLink *)fexpr->quals;
			bool		eqflg = false;

			if (sublink->operName && !strcmp(strVal(llast(sublink->operName)), "="))
				eqflg = true;

			mask_expr_node(pstate, (Expr **)&sublink->testexpr, query->rtable, true, false, eqflg);

			if (sublink->subselect && nodeTag(sublink->subselect) == T_Query)
			{
				Query *subquery = (Query *)sublink->subselect;

				if (subquery->rtable != NIL)
				{
					ListCell *lc = NULL;
					/* there is deal with return list for top query */
					if (subquery->returningList != NIL)
					{
						handle_masking(subquery->returningList, pstate,
							subquery->rtable, subquery->utilityStmt, false);
					}

					/* there is deal with target list for top query */
					if (subquery->targetList != NIL)
					{
						handle_masking(subquery->targetList, pstate,
										subquery->rtable, subquery->utilityStmt, false);
					}

					/* Deal with data redaction for subquery */
					foreach (lc, subquery->rtable)
					{
						RangeTblEntry *rte = (RangeTblEntry *)lfirst(lc);
						if (rte->rtekind == RTE_SUBQUERY && rte->subquery != NULL)
						{
							handle_masking(rte->subquery->targetList, pstate,
								rte->subquery->rtable, rte->subquery->utilityStmt, false);
						}
					}
				}
				redaction_whexception(pstate, subquery);
			}
		}
	}
}

/*
 * redaction_check
 *		return a bool value.
 *
 * The returns value has no meaning, just for code logic.
 *
 * NOTE: Find related records in table hg_redaction_column
 * according to the table oid and column name, take out the
 * value of scope from the record, and then determine the
 * return value accroding to the value of scope.
 */
static bool
redaction_check(char *polname, char *colname, int16 scope, int16 execption, bool toptlistflg, bool eqflg)
{
	bool	res = true;

	if (scope == 1)
	{
		if (execption == 16 && eqflg)
			res = false;
	}
	else if (scope == 2)
	{
		if (!toptlistflg)
			res = false;
		else if (execption == 16 && eqflg)
			res = false;
	}
	else if (scope == 4)
	{
		if (!toptlistflg && eqflg && execption == 16)
			res = false;
		else if (toptlistflg && eqflg && execption == 16)
			res = false;
		else if (!toptlistflg)
			ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
					errmsg("redaction column %s is allowed only in top targetlist",
							colname)));
	}

	return res;
}

/*
 * redaction_op_isequal
 *		return a bool value.
 *
 * NOTE: Determine whether the operator is =,
 * if the operator is "=", return true.
 */
static bool
redaction_op_isequal(OpExpr *opexpr)
{
	HeapTuple		opertuple;
	char		   *opname;
	bool			eqoperflg = false;

	if (opexpr)
	{
		/*
		 * get the operator tuple according to operator oid,
		 * and then determine which operator it is, if the
		 * operator is equal, deal with exctpion.
		 */
		opertuple = SearchSysCache1(OPEROID, ObjectIdGetDatum(opexpr->opno));
		if (HeapTupleIsValid(opertuple))
		{
			Form_pg_operator opform;

			opform = (Form_pg_operator) GETSTRUCT(opertuple);
			opname = NameStr(opform->oprname);

			/*
			 * if the operator is equal, deal with exctpion,
			 * otherwise do nothing.
			 */
			if (!strcmp(opname, "=") && (list_length(opexpr->args) == 2))
				eqoperflg = true;

			ReleaseSysCache(opertuple);
		}

	}

	return eqoperflg;
}

/*
 * process_redaction
 *		no return value.
 *
 * NOTE: data redaction according to different types.
 */
static void
process_redaction(ParseState *pstate, Query *query, bool toptlistflg, bool dealtartlist)
{
	ListCell *lc = NULL;
	
	if (query == NULL)
		return;
	
	/* 
	 * For each cte, we get its query recursively for masking,
	 * and then handle this query in normal way
	 */
	if (query->cteList != NIL)
	{
		foreach(lc, query->cteList)
		{
			CommonTableExpr *cte = (CommonTableExpr *) lfirst(lc);
			Query *cte_query = (Query *)cte->ctequery;
			process_redaction(pstate, cte_query, false, true);
		}
	}

	/* find subquery and process each subquery node */
	if (query->rtable != NULL) 
	{
		foreach(lc, query->rtable)
		{
			RangeTblEntry *rte = (RangeTblEntry *) lfirst(lc);
			Query *subquery = (Query *)rte->subquery;

			process_redaction(pstate, subquery, false, true);
		}
	}

	/* set-operation tree UNION query */
	if (query->setOperations != NULL)
	{
		SetOperationStmt *stmt = (SetOperationStmt *)query->setOperations;
		if (stmt->op == SETOP_UNION)
		{
			if (stmt->larg != NULL && 
				 IsA(stmt->larg, RangeTblRef) && 
				 pstate->p_rtable != NULL)
			{
				RangeTblRef *l = (RangeTblRef *)stmt->larg;
				if (l->rtindex > 0 && 
					l->rtindex <= list_length(pstate->p_rtable))
				{
					Query *leftmostQuery = rt_fetch(l->rtindex, pstate->p_rtable)->subquery;
					select_PostParseAnalyze(pstate, leftmostQuery, false, true);
				}
			}
			if (stmt->rarg != NULL && 
			IsA(stmt->rarg, RangeTblRef) && 
			pstate->p_rtable != NULL)
			{
				RangeTblRef *r = (RangeTblRef *)stmt->rarg;
				if (r->rtindex > 0 && r->rtindex <= list_length(pstate->p_rtable))
				{
					Query *rightmostQuery = rt_fetch(r->rtindex, pstate->p_rtable)->subquery;
					select_PostParseAnalyze(pstate, rightmostQuery, false, true);
				}
			}
		}
	} 
	else
	{
		select_PostParseAnalyze(pstate, query, toptlistflg, dealtartlist);
	}

}

/*
 * redaction_all_name
 *		return a string.
 *
 * NOTE: when use function redact_all_name,
 * returns a string with th same length as  the input.
 */
Datum
redaction_all_name(PG_FUNCTION_ARGS)
{
	Name		ns = PG_GETARG_NAME(0);
	char	   *s = NULL;
	int			len = 0;
	char	   *repeatstring = "x";
	Datum		textdatum;
	char	   *newtstring = NULL;
	
	s = pstrdup(NameStr(*ns));
	len = strlen(s);

	textdatum = DirectFunctionCall2Coll(repeat, (Oid)100, 
										PointerGetDatum(cstring_to_text(repeatstring)), 
										Int32GetDatum(len));
	newtstring = TextDatumGetCString(textdatum);

	PG_RETURN_DATUM(DirectFunctionCall1Coll(namein, (Oid)950, CStringGetDatum(newtstring)));
}

/*
 * default_time_str
 *		return a string.
 *
 * NOTE: when use function redact_all_name,
 * returns a default string.
 */
static char *
default_time_str(Var *var)
{
	char time_str[BUFFSIZE] = {0};

	switch (var->vartype)
	{
		case TIMEOID:
		case TIMETZOID:
		{
			/* Display value after time type data redaction */
			snprintf(time_str, sizeof(time_str), "00:00:00.000000+00");
			break;
		}
		case INTERVALOID:
		{
			/* Display value after time type data redaction */
			snprintf(time_str, sizeof(time_str), "00:00:00.000000");
			break;
		}
		case TIMESTAMPOID:
		{
			/* Display value after timestamp type data redaction */
			snprintf(time_str, sizeof(time_str), "1970-01-01 00:00:00.000000");
			break;
		}
		case TIMESTAMPTZOID:
		{
			/* Display value after timestamp type data redaction */
			snprintf(time_str, sizeof(time_str), "1970-01-01 00:00:00.000000+00");
			break;
		}
		default:
		{
			break;
		}
	}

	return pstrdup(time_str);
}

/*
 * redactionTransformDelTargetList
 *	return void
 *
 * NOTE: start data redaction on the target list
 * at each level.
 */
void
redactionTransformDelTargetList(ParseState *pstate, Query *query)
{
	List *targetList = NIL;

	if (!hg_redaction_data)
		return;

	if (!check_redaction_tb())
		return;

	if (IS_SYSSSO(GetUserId()))
		return;

	if (query == NULL || pstate->p_rtable == NIL)
		return;

	/* Only the target list is process here. */
	if (query->targetList != NIL)
		targetList = query->targetList;
	else
		return;

	/* data redaction */
	handle_masking(targetList, pstate, pstate->p_rtable, NULL, true);

	return;
}
