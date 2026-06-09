/*-------------------------------------------------------------------------
 *
 * File:parse_connectby.c
 *
 * Abstract:
 *		Handle connect by
 *
 *
 * ReqID:SRS-SQL-HierarchicalQueries
 *
 * Authored by dwdai@highgo.com,20241230.
 *
 * Copyright:
 * Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  src/backend/parser/parse_connectby.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "miscadmin.h"
#include "parser/parsetree.h"
#include "parser/parse_connectby.h"
#include "parser/parse_expr.h"
#include "parser/parse_clause.h"
#include "parser/parse_relation.h"
#include "parser/analyze.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "catalog/namespace.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"
#include "utils/ruleutils.h"
#include "utils/guc.h"
#include "commands/dbcommands.h"
#include "executor/spi.h"
#include "optimizer/optimizer.h"
#include "catalog/pg_collation.h"


#define CONNECT_BY_CONTAIN_LEVEL 0x01
#define CONNECT_BY_CONTAIN_ROWNUM 0x02
#define CONNECT_BY_CONTAIN_LEAF	0x04
#define CONNECT_BY_CONTAIN_CYCLE 0x08
#define CONNECT_BY_REPLACE_LEVEL  0x10

#define PRE_NO_OPT 0x00
#define PRE_AND_OPT 0x01
#define PRE_OR_OPT 0x02


static Node* makeColumnRef(char *colname, List *indirection);
static Node* makeIntConst(int val);
static Node* makeSetOp(SetOperation op, bool all, Node *larg, Node *rarg);
static Node* makeAndExpr(Node *lexpr, Node *rexpr, int location);
static Node *makeOrExpr(Node *lexpr, Node *rexpr, int location);
static Node* makeStringConst(char *str);
static Node* makeTypeCast(Node *arg, TypeName *typename, int location);
static Node* makeBoolAConst(bool state);

static Node *parsetree_tree_walker(Node *pnode, Node* (*walker) (),
								   void *context);
/*
 * replace pseudocol context
 */
typedef struct replace_pseudocol_context
{
	char *finalname;
	char *connect_by_root;		/* connect_by_path */
	char *connect_by_path;		/* connect_by_path */
	char *connect_by_iscycle;	/* connect_by_iscycle */
	char *connect_by_isleaf;	/* connect_by_isleaf */
	char *levelcol;				/* level */
	int *flags;					/* flags */
	List **rootlist;			/* the list of connect_by_root */
	List **path_list;			/* the list of sys_connect_by_path */
} replace_pseudocol_context;


static Node *replace_pseduocol_walker(Node *node, void *context);

/*
 * check target dbname context
 */
typedef struct check_target_tbname_context
{
	ParseState *cur_pstate;	/* current ParseState */
	char *tbname;		/* current check tbname */
	/* Begin - bug#00008247 */
	int sublevel;		/* subselect level */
	ParseState *child_pstate;	/* child ParseState */
	/* End - bug#00008247 */
} check_target_tbname_context;

static Node *check_target_tname_walker(Node *node, void *context);


/*
 * add table reference context
 */
typedef struct add_tabreference_context
{
	ParseState *cur_pstate;
	char *tbname;
	bool remove_schema;
	/* Begin - BUG#Z00005447 */
	bool	only_replace;		/* only replace a.xx to tbname.xx */
	/* End - BUG#Z00005447 */
	ParseState *child_pstate;
	bool child_hasconnectby;
	/* Begin - bug#00008247 */
	int	sublevel;		/* subselect level */
	/* End - bug#00008247 */
} add_tabreference_context;

static Node *add_tabreference_walker(Node *node, void *context);

/*
 * replace priorcolname context
 */
typedef struct  replace_priorcolname_context
{
	char *prior_colname_prefix;
	List **resultlist;
	bool found_prior;
	int *flags;
} replace_priorcolname_context;

static Node *replace_priorcolname_walker(Node *node, void *context);

static Node *remove_outjoin_walker(Node *node, void *context);

/*
 * replace series context
 */
typedef struct replace_series_context
{
	ParseState *cur_pstate;
	int *recurse_flag;
	bool *found_prior;
	bool pre_prior;
	char *tbname;
	char *prior_name;
	List **prior_columlist;
	ParseState *child_pstate;
	bool child_hasconnectby;
	/* Begin - bug#00008247 */
	int sublevel;	/* subselect levle */
	/* End - bug#00008247 */
} replace_series_context;


static Node *replace_series_walker(Node *node, void *context);

static Node *replace_rownum_to_param_walker(Node *node, void *context);


/*
 * remove table reference context
 */
typedef struct remove_tabreference_context
{
	ParseState *cur_pstate;
	char *tbname;
	List **columnlist;
	char *prefix_str;
	ParseState *child_pstate;
	/* Begin - bug#00008247 */
	bool only_replace;	/* is only replace table reference */
	int sublevel;	/* subselect levle */
	/* End - bug#00008247 */
} remove_tabreference_context;


static Node *remove_tabreference_walker(Node *node, void *context);

/*
 * replace orderby aliasname context
 */
typedef struct replace_orderby_aliasname_context
{
	List *targetlist;
} replace_orderby_aliasname_context;

static void replace_orderby_aliasname(ParseState *cur_pstate, List *targetlist, List *sortclause);
static Node *replace_orderby_aliasname_walker(Node *node, void *context);


/*
 * split join where clause context
 */
typedef struct split_join_where_clause_context
{
	ParseState *cur_pstate;
	List **table_indexs;
	bool replace_join;
	int flags;
	bool *has_prior;
	bool has_outerjoin;
	JoinExpr *je;
	bool *internel_outerjoin;
	List **outjoin_indexs;
	bool twice_replace_outjoin;
}split_join_where_clause_context;

static bool split_join_where_clause_recurse(Node **pnode, split_join_where_clause_context *context);
static bool is_bool_operator(List *operators);
static List *append_table_columnlist(List *table_name, List *prior_column);


static Oid getColumnTypeidFromColumnref(ParseState *cur_pstate,
									ColumnRef *ref, int32 *colmod,
									Oid *colcollaction);
static List *expandTargetListAstar(ParseState *cur_pstate, List *oldtargets);
static List *get_collation_name_forlist(Oid colloid);



/*
 * next operators return bool
 */
static int bool_operators_len = 16;
static char *bool_operators[] = {
	"<", ">", "=", "<=", ">=", "<>",
	"~~", "!~~", "~~*", "!~~*", "~",
	"!~", "BETWEEN", "NOT BETWEEN",
	"BETWEEN SYMMETRIC", "NOT BETWEEN SYMMETRIC"
};


/*
 * Transformation of the same name function becomes, but
 * it make a simple Column
 */
static Node*
makeColumnRef(char *colname, List *indirection)
{
	ColumnRef  *c = makeNode(ColumnRef);

	c->fields = lcons(makeString(colname), indirection);
	c->location = -1;

	return (Node *) c;
}


/*
 * make IntConst
 */
static Node*
makeIntConst(int val)
{
	A_Const *n = makeNode(A_Const);

	n->val.type = T_Integer;
	n->val.val.ival = val;
	n->location = -1;

	return (Node *)n;
}


/*
 * make a simiple setop
 */
static Node*
makeSetOp(SetOperation op, bool all, Node *larg, Node *rarg)
{
	SelectStmt *n = makeNode(SelectStmt);

	n->op = op;
	n->all = all;
	n->larg = (SelectStmt *) larg;
	n->rarg = (SelectStmt *) rarg;

	return (Node *) n;
}

/*
 * make a simple andExpr
 */
static Node*
makeAndExpr(Node *lexpr, Node *rexpr, int location)
{
	Node	   *lexp = lexpr;

	/* Flatten "a AND b AND c ..." to a single BoolExpr on sight */
	if (IsA(lexp, BoolExpr))
	{
		BoolExpr *blexpr = (BoolExpr *)lexp;

		if (blexpr->boolop == AND_EXPR)
		{
			blexpr->args = lappend(blexpr->args, rexpr);
			return (Node *)blexpr;
		}
	}
	return (Node *) makeBoolExpr(AND_EXPR, list_make2(lexpr, rexpr),
			false, location);
}

static Node *
makeOrExpr(Node *lexpr, Node *rexpr, int location)
{
	Node	   *lexp = lexpr;

	/* Flatten "a OR b OR c ..." to a single BoolExpr on sight */
	if (IsA(lexp, BoolExpr))
	{
		BoolExpr *blexpr = (BoolExpr *) lexp;

		if (blexpr->boolop == OR_EXPR)
		{
			blexpr->args = lappend(blexpr->args, rexpr);
			return (Node *) blexpr;
		}
	}
	return (Node *) makeBoolExpr(OR_EXPR, list_make2(lexpr, rexpr),
				false, location);
}


static Node*
makeStringConst(char *str)
{
	A_Const *n = makeNode(A_Const);

	n->val.type = T_String;
	n->val.val.str = str;

	return (Node *)n;
}

static Node*
makeTypeCast(Node *arg, TypeName *typename, int location)
{
	TypeCast *n = makeNode(TypeCast);

	n->arg = arg;
	n->typeName = typename;
	n->location = location;

	return (Node *)n;
}



static Node*
makeBoolAConst(bool state)
{
	A_Const *n = makeNode(A_Const);

	n->val.type = T_String;
	n->val.val.str = (state ? "t" : "f");

	return makeTypeCast((Node *)n, SystemTypeName("bool"), -1);
}


/*
 * parset tree walker
 */
static Node *
parsetree_tree_walker(Node *node, Node* (*walker) (),
								   void *context)
{
	Node *result = node;

	if (node == NULL)
		return result;

	/* Guard against stack overflow due to overly complex expressions */
	check_stack_depth();

	switch (nodeTag(node))
	{
		case T_ResTarget:
			{
				ResTarget *res = (ResTarget *) node;

				res->val = parsetree_tree_walker(res->val, walker, context);
				break;
			}
			break;
		case T_ColumnRef:
		case T_SysConnectPath:
		case T_ConnectByIsCycle:
		case T_ConnectByRoot:
		case T_SelectStmt:
		case T_PriorAexpr:
		case T_PseudoColumnRef:
			result = walker(node, context);
			break;
		case T_ParamRef:
		case T_OraParamRef:
			break;
		case T_A_Const:
			break;
		case T_A_Indirection:
			{
				A_Indirection *ind = (A_Indirection *) node;

				ind->arg = parsetree_tree_walker(ind->arg, walker, context);
				break;
			}

		case T_A_ArrayExpr:
			break;

		case T_TypeCast:
			((TypeCast *) node)->arg = parsetree_tree_walker(((TypeCast *) node)->arg, walker, context);
			break;

		case T_CollateClause:
			((CollateClause *) node)->arg = parsetree_tree_walker(((CollateClause *) node)->arg, walker, context);
			break;

		case T_A_Expr:
			{
				A_Expr	   *a = (A_Expr *) node;

				a->lexpr = parsetree_tree_walker(a->lexpr, walker, context);
				a->rexpr = parsetree_tree_walker(a->rexpr, walker, context);
				break;
			}
		case T_List:
			{
				ListCell *lc;
				List *targetlist = (List *) node;

				foreach (lc, targetlist)
				{
					Node *n = (Node *) lfirst(lc);

					lfirst(lc) = parsetree_tree_walker(n, walker, context);
				}
				break;
			}

		case T_BoolExpr:
			((BoolExpr *) node)->args = (List *) parsetree_tree_walker((Node *) ((BoolExpr *) node)->args, walker, context);
			break;

		case T_FuncCall:
			{
				/* Begin - bug#00008247 */
				FuncCall *fc = (FuncCall *) node;

				fc->args = (List *) parsetree_tree_walker((Node *) fc->args, walker, context);
				fc->agg_filter = parsetree_tree_walker(fc->agg_filter, walker, context);
				fc->agg_order = (List *) parsetree_tree_walker((Node *) fc->agg_order, walker, context);
				fc->over = (struct WindowDef *) parsetree_tree_walker((Node *) fc->over, walker, context);
				/* End - bug#00008247 */
				break;
			}

		case T_ColumnRefOrFuncCall:
			{
				/*
				 * there cref maybe replace by other node, so we should return its replace node
				 */
				ColumnRefOrFuncCall * colref = (ColumnRefOrFuncCall *) node;
				ColumnRef *oldcref = colref->cref;

				colref->cref = (ColumnRef *)
									parsetree_tree_walker((Node *) colref->cref,
									walker, context);
				if (oldcref != colref->cref)
					result = (Node *) colref->cref;
				break;
			}

		case T_MultiAssignRef:
			((MultiAssignRef *) node)->source = parsetree_tree_walker(((MultiAssignRef *) node)->source,
													walker, context);
			break;

		case T_GroupingFunc:
			{
				GroupingFunc *gpf = (GroupingFunc *) node;

				gpf->args = (List *) parsetree_tree_walker((Node *) gpf->args, walker, context);
				gpf->refs = (List *) parsetree_tree_walker((Node *) gpf->refs, walker, context);
				gpf->cols = (List *) parsetree_tree_walker((Node *) gpf->cols, walker, context);
				break;
			}

		case T_NamedArgExpr:
			{
				NamedArgExpr *nexpr = (NamedArgExpr *) node;

				nexpr->arg = (Expr *) parsetree_tree_walker((Node *)nexpr->arg, walker, context);
				break;
			}
		case T_SubLink:
			{
				SubLink *sblk = (SubLink *) node;

				sblk->testexpr = parsetree_tree_walker(sblk->testexpr, walker, context);
				sblk->subselect = parsetree_tree_walker(sblk->subselect, walker, context);
				break;
			}

		case T_CaseExpr:
			((CaseExpr *) node)->arg = (Expr *) parsetree_tree_walker((Node *)((CaseExpr *) node)->arg,
													walker, context);

			((CaseExpr *) node)->args = (List *) parsetree_tree_walker((Node *)((CaseExpr *) node)->args, walker, context);

			((CaseExpr *) node)->defresult = (Expr *) parsetree_tree_walker((Node *)((CaseExpr *) node)->defresult,
													walker, context);
			break;

		case T_RowExpr:
			((RowExpr *) node)->args = (List *) parsetree_tree_walker((Node *) ((RowExpr *) node)->args, walker, context);
			break;

		case T_CoalesceExpr:
			((CoalesceExpr *) node)->args = (List *) parsetree_tree_walker((Node *) ((CoalesceExpr *) node)->args,
													walker, context);
			break;

		case T_MinMaxExpr:
			((MinMaxExpr *) node)->args = (List *) parsetree_tree_walker((Node *) ((MinMaxExpr *) node)->args,
													walker, context);
			break;

		case T_XmlExpr:
			{
				XmlExpr *xml = (XmlExpr *) node;

				xml->arg_names = (List *) parsetree_tree_walker((Node *) xml->arg_names,
											walker, context);
				xml->named_args = (List *) parsetree_tree_walker((Node *) xml->named_args,
											walker, context);
				xml->args = (List *) parsetree_tree_walker((Node *) xml->args,
											walker, context);
				break;
			}

		case T_XmlSerialize:
			{
				XmlSerialize *xmls = (XmlSerialize *) node;

				xmls->expr = parsetree_tree_walker(xmls->expr,
								walker, context);
				break;
			}

		case T_NullTest:
			{
				NullTest   *n = (NullTest *) node;

				n->arg = (Expr *) parsetree_tree_walker((Node *) n->arg, walker, context);
				break;
			}

		case T_BooleanTest:
			((BooleanTest *) node)->arg = (Expr *) parsetree_tree_walker((Node *) ((BooleanTest *) node)->arg,
													walker, context);
			break;

		case T_CurrentOfExpr:
		case T_CaseTestExpr:
		case T_SetToDefault:
		case T_Var:
		case T_Const:	/* BUGID:504 seq.nextval and seq.currval */
			break;
		case T_SortBy:
			{
				SortBy *sb = (SortBy *) node;

				sb->node = parsetree_tree_walker(sb->node, walker, context);
				break;
			}
		case T_CaseWhen:
			{
				CaseWhen *cs = (CaseWhen *) node;

				cs->expr = (Expr *) parsetree_tree_walker((Node *) cs->expr, walker, context);
				cs->orig_expr = (Expr *) parsetree_tree_walker((Node *) cs->orig_expr,
														walker, context);
				cs->result = (Expr *) parsetree_tree_walker((Node *) cs->result,
														walker, context);
				break;
			}
		/* Begin - BUG#7712 */
		case T_OraRowExpr:
			((OraRowExpr *) node)->args = (List *) parsetree_tree_walker((Node *) ((OraRowExpr *) node)->args,
																walker, context);
			break;
		/* End - BUG#7712 */
		/* Begin - bug#00008247 */
		case T_WindowDef:
			{
				WindowDef *wf = (WindowDef *) node;

				wf->partitionClause = (List *) parsetree_tree_walker((Node *) wf->partitionClause, walker, context);
				wf->orderClause = (List *) parsetree_tree_walker((Node *) wf->orderClause, walker, context);
				wf->startOffset = parsetree_tree_walker( wf->startOffset, walker, context);
				wf->endOffset = parsetree_tree_walker(wf->endOffset, walker, context);

				break;
			}
		/* End - bug#00008247 */
		default:
			break;
	}

	return result;
}


/*
 * replcae pseudo column to columnref
 */
static Node*
replace_pseduocol_walker(Node *node, void *context)
{
	Node *result = node;
	replace_pseudocol_context *ctx = (replace_pseudocol_context *) context;

	if (node == NULL)
		return result;

	switch (nodeTag(node))
	{
		case T_ColumnRef:
			{
				ColumnRef *r = (ColumnRef *) node;

				if (list_length(r->fields) == 1)
				{
					Node *strnode;

					strnode = linitial(r->fields) ;
					if (T_String == nodeTag(strnode))
					{
						if (strcmp(ctx->connect_by_isleaf, strVal(strnode)) == 0)
							*ctx->flags |= CONNECT_BY_CONTAIN_LEAF;
						else if (strcmp(ctx->levelcol, strVal(strnode)) == 0)
							*ctx->flags |= CONNECT_BY_CONTAIN_LEVEL;
					}
				}
				break;
			}
		case T_SysConnectPath:
			{
				SysConnectPath *syspath = (SysConnectPath *) node;
				SysConnectPath *stmp;
				bool found = false;
				ListCell *lc;

				result = makeColumnRef(ctx->finalname, list_make1(makeString(syspath->alias)));
				foreach (lc, *ctx->path_list)
				{
					stmp = (SysConnectPath *) lfirst(lc);
					if (strcmp(stmp->alias, syspath->alias) == 0)
					{
						found = true;
						break;
					}
				}
				if (!found)
					*ctx->path_list = lappend(*ctx->path_list, syspath);
				break;
			}
		case T_ConnectByIsCycle:
			result = makeColumnRef(ctx->finalname, list_make1(makeString(ctx->connect_by_iscycle)));
			*ctx->flags |= CONNECT_BY_CONTAIN_CYCLE;
			break;
		case T_ConnectByRoot:
			{
				ConnectByRoot *croot = (ConnectByRoot *) node;
				ConnectByRoot *ctmp;
				bool found = false;
				ListCell *lc;

				result = makeColumnRef(ctx->finalname, list_make1(makeString(croot->alias)));
				foreach (lc, *ctx->rootlist)
				{
					ctmp = (ConnectByRoot *) lfirst(lc);
					if (strcmp(ctmp->alias, croot->alias) == 0)
					{
						found = true;
						break;
					}
				}
				if (!found)
					*ctx->rootlist = lappend(*ctx->rootlist, croot);
				break;
			}
		case T_SelectStmt:
		case T_PriorAexpr:
		case T_PseudoColumnRef:
			break;
		default:
			elog(ERROR, "unrecognized node type: %d", (int) nodeTag(node));
			break;
	}

	return result;
}


/*
 * checck target tname walker
 */
static Node *
check_target_tname_walker(Node *node, void *context)
{
	Node *result = node;
	check_target_tbname_context *ctx = (check_target_tbname_context *) context;

	if (node == NULL)
		return result;

	switch (nodeTag(node))
	{
		case T_ColumnRef:
			{
				ColumnRef *r = (ColumnRef *) node;
				bool	connectby_col = false;

				/* ignore * */
				if (IsA(llast(r->fields), A_Star))
					connectby_col = true;
				else
				{
					/* Begin - bug#00008247 */
					if (ctx->child_pstate != NULL)
					{
						Node *tnode;
						ColumnRef *tmpref = copyObject(r);

						tnode = IvyTransformColumnRef(ctx->child_pstate, tmpref, true);

						if (tnode == NULL ||
							nodeTag(tnode) != T_Var ||
							((Var *) tnode)->varlevelsup == ctx->sublevel)
							connectby_col = true;

						if (tnode != NULL)
							pfree(tnode);
						pfree(tmpref);
					}
					else
						connectby_col = true;

					if (connectby_col && ctx->cur_pstate != NULL)
					{
						Node *tnode;
						ColumnRef *tmpref = copyObject(r);

						tnode = IvyTransformColumnRef(ctx->cur_pstate, tmpref, true);

						if (tnode != NULL &&
							(nodeTag(tnode) != T_Var||
							((Var *) tnode)->varlevelsup != 0))
							connectby_col = false;

						if (tnode != NULL)
							pfree(tnode);
						pfree(tmpref);
					}
					/* End - bug#00008247 */
				}

				if (connectby_col && list_length(r->fields) > 1)
				{
					Node *filednode;

					filednode = (Node *) lfirst(list_nth_cell(r->fields,
									list_length(r->fields) - 2));

					if (T_String == nodeTag(filednode))
					{
						char *tmp = strVal(filednode);

						if (0 != strcmp(ctx->tbname, tmp))
						{
							ereport(ERROR,
									(errcode(ERRCODE_SYNTAX_ERROR),
									errmsg("the target list name:%s and the from clause name:%s must"
									" be the same",
									tmp, ctx->tbname)));
						}
					}
				}
				break;
			}
		case T_SysConnectPath:
			{
				SysConnectPath *syspath = (SysConnectPath *) node;

				syspath->colnode = parsetree_tree_walker(syspath->colnode, check_target_tname_walker, context);
				break;
			}
		case T_ConnectByIsCycle:
			break;
		case T_ConnectByRoot:
			{
				ConnectByRoot *croot = (ConnectByRoot *) node;

				croot->colnode = parsetree_tree_walker(croot->colnode, check_target_tname_walker, context);
				break;
			}
		case T_SelectStmt:
			{
				SelectStmt *stmt = (SelectStmt *) node;
				ParseState *newpstate;
				ParseState *saved_state;
				/* Begin - BUG#Z00005447 */
				List *from_clause;
				/* End - BUG#Z00005447 */
	
				newpstate = make_parsestate(ctx->child_pstate == NULL ? ctx->cur_pstate : ctx->child_pstate); /* bug#00008247 */
				newpstate->p_sourcetext = NULL;

				/* Begin - BUG#Z00005447 */
				/*
				 * there, transformFromClause maybe change origin
				 * fromclauseitem like JoinExpr, so used in later
				 * mabye report error, so we copy it
				 */
				from_clause = copyObject(stmt->fromClause);
				transformFromClause(newpstate, from_clause);
				list_free(from_clause);
				/* End - BUG#Z00005447 */
				newpstate->p_expr_kind = EXPR_KIND_SELECT_TARGET;

				/* Begin - bug#00008247 */
				saved_state = ctx->child_pstate;
				ctx->child_pstate = newpstate;
				ctx->sublevel++;
				/* End - bug#00008247 */

				parsetree_tree_walker((Node *) stmt->targetList, check_target_tname_walker,
											context);
				newpstate->p_expr_kind = EXPR_KIND_WHERE;
				parsetree_tree_walker(stmt->whereClause, check_target_tname_walker,
									context);
				newpstate->p_expr_kind = EXPR_KIND_GROUP_BY;
				parsetree_tree_walker((Node *) stmt->sortClause, check_target_tname_walker,
									context);
				newpstate->p_expr_kind = EXPR_KIND_LIMIT;
				parsetree_tree_walker(stmt->limitOffset, check_target_tname_walker,
									context);
				parsetree_tree_walker(stmt->limitCount, check_target_tname_walker,
									context);
				parsetree_tree_walker((Node *) stmt->lockingClause, check_target_tname_walker,
									context);
				newpstate->p_expr_kind = EXPR_KIND_GROUP_BY;
				parsetree_tree_walker((Node *) stmt->groupClause, check_target_tname_walker,
									context);
				newpstate->p_expr_kind = EXPR_KIND_HAVING;
				parsetree_tree_walker(stmt->havingClause, check_target_tname_walker,
									context);
				free_parsestate(newpstate);
				/* Begin - bug#00008247 */
				ctx->child_pstate = saved_state;
				ctx->sublevel--;
				/* End - bug#00008247 */
				break;
			}
		case T_PriorAexpr:
			{
				PriorAexpr *aexpr = (PriorAexpr*) node;

				aexpr->arg = parsetree_tree_walker(aexpr->arg, check_target_tname_walker, context);
				break;
			}
		case T_PseudoColumnRef:
			break;
		default:
			elog(ERROR, "unrecognized node type: %d", (int) nodeTag(node));
			break;
	}

	return result;
}


/*
 * add tabreference walker
 */
static Node *
add_tabreference_walker(Node *node, void *context)
{
	Node *result = node;
	add_tabreference_context *ctx = (add_tabreference_context *) context;

	if (node == NULL)
		return result;

	switch (nodeTag(node))
	{
		case T_ColumnRef:
			{
				ColumnRef *r = (ColumnRef *) node;
				Node *filednode;
				bool	connectby_col = false;

				/* ignore * */
				if (IsA(llast(r->fields), A_Star))
					connectby_col = true;
				else
				{
					if (ctx->child_pstate != NULL)
					{
						Node *tnode;
						ColumnRef *tmpref = copyObject(r);

						tnode = IvyTransformColumnRef(ctx->child_pstate, tmpref, true);

						/* Begin - bug#00008247 */
						if (tnode == NULL ||
							nodeTag(tnode) != T_Var ||
							((Var *) tnode)->varlevelsup == ctx->sublevel)
							connectby_col = true;
						/* End - bug#00008247 */
						if (tnode != NULL)
							pfree(tnode);
						pfree(tmpref);
					}
					else
						connectby_col = true;

					if (connectby_col && ctx->cur_pstate != NULL)
					{
						/* maybe out level col or variable */
						Node *tnode;
						ColumnRef *tmpref = copyObject(r);

						tnode = IvyTransformColumnRef(ctx->cur_pstate, tmpref, true);

						if (tnode != NULL &&
							(nodeTag(tnode) != T_Var ||
							((Var *) tnode)->varlevelsup != 0))
							connectby_col = false;

						if (tnode != NULL)
							pfree(tnode);
						pfree(tmpref);
					}
				}

				if (connectby_col)
				{
					switch (list_length(r->fields))
					{
						case 1:
							/*
							 * if we in a subquery
							 *   and it has a connect by, we
							 *   should not replace level and no handle
							 *   if not has a connectby,we should raise error
							 *   or we will use the parent connect by'level
							 *   which is wrong
							 */
							if (ctx->child_pstate != NULL &&
								strcmp(strVal(linitial(r->fields)), "level") == 0)
							{
								if (ctx->child_hasconnectby)
									break;
								else
									elog(ERROR, "CONNECT BY clause required in this query block");
							}
							/* Begin - BUG#Z00005447 */
							/* final add_replace should only replace for some alias columns */
							if (!ctx->only_replace)
								r->fields = lcons(makeString(pstrdup(ctx->tbname)),
										r->fields);
							/* End - BUG#Z00005447 */
							break;
						case 2:
							filednode = linitial(r->fields);
							strVal(filednode) = ctx->tbname;
							break;
						case 3:
							filednode = lsecond(r->fields);
							strVal(filednode) = ctx->tbname;
							if (ctx->remove_schema)
								r->fields = list_delete_first(r->fields);
							break;
						default:
							filednode = (Node *) lfirst(list_nth_cell(r->fields,
												list_length(r->fields) - 2));
							strVal(filednode) = ctx->tbname;
							if (ctx->remove_schema)
							{
								r->fields = list_delete_first(r->fields);
								r->fields = list_delete_first(r->fields);
							}
							break;
					}
				}
				break;
			}
		case T_SysConnectPath:
			{
				SysConnectPath *syspath = (SysConnectPath *) node;

				syspath->colnode = parsetree_tree_walker(syspath->colnode, add_tabreference_walker, context);
				break;
			}
		case T_ConnectByIsCycle:
			break;
		case T_ConnectByRoot:
			{
				ConnectByRoot *croot = (ConnectByRoot *) node;

				croot->colnode = parsetree_tree_walker(croot->colnode, add_tabreference_walker, context);
				break;
			}
		case T_SelectStmt:
			{
				SelectStmt *stmt = (SelectStmt *) node;
				ParseState *newpstate;
				bool child_has_connectby = (stmt->connect_by != NULL);
				bool saved_has_connectby;
				ParseState *save_child_pstate;
				/* Begin - BUG#Z00005447 */
				List		*copy_fromclause = NIL;
				/* End - BUG#Z00005447 */

				newpstate = make_parsestate(ctx->child_pstate == NULL ? ctx->cur_pstate : ctx->child_pstate); /* bug#00008247 */
				newpstate->p_sourcetext = NULL;

				/* Begin - BUG#Z00005447 */
				/*
				 * there, transformFromClause maybe change origin
				 * fromclauseitem like JoinExpr, so used in later
				 * mabye report error, so we copy it
				 */
				copy_fromclause = copyObject(stmt->fromClause);
				transformFromClause(newpstate, copy_fromclause);
				list_free(copy_fromclause);
				/* End - BUG#Z00005447 */
				newpstate->p_expr_kind = EXPR_KIND_SELECT_TARGET;

				saved_has_connectby = ctx->child_hasconnectby;
				save_child_pstate = ctx->child_pstate;

				ctx->child_hasconnectby = child_has_connectby;
				ctx->child_pstate = newpstate;
				ctx->sublevel++; /* bug#00008247 */

				stmt->targetList = (List *) parsetree_tree_walker((Node *) stmt->targetList,
									add_tabreference_walker, context);
				newpstate->p_expr_kind = EXPR_KIND_WHERE;
				stmt->whereClause = parsetree_tree_walker(stmt->whereClause,
							add_tabreference_walker, context);
				newpstate->p_expr_kind = EXPR_KIND_GROUP_BY;
				stmt->sortClause = (List *) parsetree_tree_walker((Node *) stmt->sortClause,
						add_tabreference_walker, context);
				newpstate->p_expr_kind = EXPR_KIND_LIMIT;
				stmt->limitOffset = parsetree_tree_walker(stmt->limitOffset,
								add_tabreference_walker, context);
				stmt->limitCount = parsetree_tree_walker(stmt->limitCount,
					add_tabreference_walker, context);
				stmt->lockingClause = (List *) parsetree_tree_walker((Node *) stmt->lockingClause,
					add_tabreference_walker, context);
				newpstate->p_expr_kind = EXPR_KIND_GROUP_BY;
				stmt->groupClause = (List *) parsetree_tree_walker((Node *) stmt->groupClause,
					add_tabreference_walker, context);
				newpstate->p_expr_kind = EXPR_KIND_HAVING;
				stmt->havingClause = parsetree_tree_walker(stmt->havingClause,
					add_tabreference_walker, context);
				free_parsestate(newpstate);

				ctx->child_hasconnectby = saved_has_connectby;
				ctx->child_pstate = save_child_pstate;
				ctx->sublevel--; /* bug#00008247 */
				break;
			}
		case T_PriorAexpr:
		case T_PseudoColumnRef:
			break;
		default:
			elog(ERROR, "unrecognized node type: %d", (int) nodeTag(node));
			break;
	}

	return result;
}



/*
 * remove table reference walker
 *
 * remove table.xxx to prefix_str.table.xxx
 *
 * where_clause support correlated subquery
 * so we add pstate parameter
 */
static Node*
remove_tabreference_walker(Node *node, void *context)
{
	Node *result = node;
	remove_tabreference_context *ctx = (remove_tabreference_context *) context;

	if (node == NULL)
		return result;

	switch (nodeTag(node))
	{
		case T_ColumnRef:
			{
				ColumnRef *r = (ColumnRef *) node;
				bool	connectby_col = false;

				/* ignore * */
				if (IsA(llast(r->fields), A_Star))
					connectby_col = false;
				else
				{
					if (ctx->child_pstate != NULL)
					{
						Node *tnode;
						ColumnRef *tmpref = copyObject(r);

						tnode = IvyTransformColumnRef(ctx->child_pstate, tmpref, true);

						/* Begin - bug#00008247 */
						if (tnode == NULL ||
							(nodeTag(tnode) != T_Var ||
							((Var *) tnode)->varlevelsup == ctx->sublevel))
							connectby_col = true;
						/* End - bug#00008247 */
						if (tnode != NULL)
							pfree(tnode);
						pfree(tmpref);
					}
					else
						connectby_col = true;

					if (connectby_col && ctx->cur_pstate != NULL)
					{
						/* maybe out level col or variable */
						Node *tnode;
						ColumnRef *tmpref = copyObject(r);

						tnode = IvyTransformColumnRef(ctx->cur_pstate, tmpref, true);

						if (tnode != NULL  &&
							(nodeTag(tnode) != T_Var ||
							((Var *) tnode)->varlevelsup != 0))
							connectby_col = false;
						if (tnode)
							pfree(tnode);
						pfree(tmpref);
					}
				}

				if (connectby_col)
				{
					if (list_length(r->fields) > 1)
					{
						char *nameStr;
						ListCell *lc;
						StringInfoData strinfo;

						nameStr = NameListToString(r->fields);

						if (ctx->columnlist != NULL)
						{
							bool found = false;

							foreach (lc, *ctx->columnlist)
							{
								ColumnRef *rt = (ColumnRef *) lfirst(lc);
								char *strtmp;

								strtmp = NameListToString(rt->fields);

								if (strcmp(strtmp, nameStr) == 0)
								{
									found = true;
									pfree(strtmp);
									break;
								}
								pfree(strtmp);
							}
							if (!found)
								*ctx->columnlist = lappend(*ctx->columnlist, copyObject(r));
						}
						initStringInfo(&strinfo);
						appendStringInfo(&strinfo, "%s%s", ctx->prefix_str, nameStr);
						pfree(nameStr);
						r->fields = list_make2(makeString(pstrdup(ctx->tbname)),
									makeString(strinfo.data));
					}
					else if (list_length(r->fields) == 1 &&
						!ctx->only_replace) /* bug#00008247 */
					{
						Node *strnode;

						strnode = linitial(r->fields) ;
						if (T_String == nodeTag(strnode) &&
							pg_strcasecmp(strVal(strnode), "level") != 0)
							r->fields = lcons(makeString(pstrdup(ctx->tbname)), r->fields);
						else if (T_String != nodeTag(strnode))
							r->fields = lcons(makeString(pstrdup(ctx->tbname)), r->fields);
					}
				}
				break;
			}
		case T_SysConnectPath:
			{
				SysConnectPath *syspath = (SysConnectPath *) node;

				syspath->colnode = parsetree_tree_walker(syspath->colnode, remove_tabreference_walker, context);
				break;
			}
		case T_ConnectByIsCycle:
			break;
		case T_ConnectByRoot:
			{
				ConnectByRoot *croot = (ConnectByRoot *) node;

				croot->colnode = parsetree_tree_walker(croot->colnode, remove_tabreference_walker, context);
				break;
			}
		case T_SelectStmt:
			{
				SelectStmt *stmt = (SelectStmt *) node;
				ParseState *newpstate;
				ParseState *saved_child_pstate = ctx->child_pstate;
				/* Begin - BUG#Z00005447 */
				List *from_clause;
				/* End - BUG#Z00005447 */

				newpstate = make_parsestate(ctx->child_pstate == NULL ? ctx->cur_pstate : ctx->child_pstate); /* bug#00008247 */
				newpstate->p_sourcetext = NULL;

				ctx->child_pstate = newpstate;
				ctx->sublevel++; /* bug#00008247 */

				/* Begin - BUG#Z00005447 */
				/*
				 * there, transformFromClause maybe change origin
				 * fromclauseitem like JoinExpr, so used in later
				 * mabye report error, so we copy it
				 */
				from_clause = copyObject(stmt->fromClause);
				transformFromClause(newpstate, from_clause);
				list_free(from_clause);
				/* End - BUG#Z00005447 */
				newpstate->p_expr_kind = EXPR_KIND_SELECT_TARGET;
				stmt->targetList = (List *) parsetree_tree_walker((Node *)stmt->targetList,
								remove_tabreference_walker, context);

				newpstate->p_expr_kind = EXPR_KIND_WHERE;
				stmt->whereClause = parsetree_tree_walker(stmt->whereClause,
								remove_tabreference_walker, context);

				newpstate->p_expr_kind = EXPR_KIND_GROUP_BY;
				stmt->sortClause = (List *) parsetree_tree_walker((Node *) stmt->sortClause,
								remove_tabreference_walker, context);

				newpstate->p_expr_kind = EXPR_KIND_LIMIT;
				stmt->limitOffset = parsetree_tree_walker(stmt->limitOffset,
								remove_tabreference_walker, context);

				stmt->limitCount = parsetree_tree_walker(stmt->limitCount,
								remove_tabreference_walker, context);
				stmt->lockingClause = (List *) parsetree_tree_walker((Node *) stmt->lockingClause,
									remove_tabreference_walker, context);

				newpstate->p_expr_kind = EXPR_KIND_GROUP_BY;
				stmt->groupClause = (List *) parsetree_tree_walker((Node *)stmt->groupClause,
							remove_tabreference_walker, context);

				newpstate->p_expr_kind = EXPR_KIND_HAVING;
				stmt->havingClause = parsetree_tree_walker(stmt->havingClause,
							remove_tabreference_walker, context);
				free_parsestate(newpstate);

				ctx->child_pstate = saved_child_pstate;
				ctx->sublevel--; /* bug#00008247 */
				break;
			}
		case T_PriorAexpr:
			{
				PriorAexpr *priorexp = (PriorAexpr *) node;

				priorexp->arg = parsetree_tree_walker(priorexp->arg,
							remove_tabreference_walker, context);
				break;
			}
		case T_PseudoColumnRef:
			break;
		default:
			elog(ERROR, "unrecognized node type: %d", (int) nodeTag(node));
			break;
	}

	return result;
}


/*
 * wether order by using col aliasname, we should
 * replace it to real col name
 */
static void
replace_orderby_aliasname(ParseState *cur_pstate, List *targetlist, List *sortclause)
{
	ListCell *lc;
	replace_orderby_aliasname_context context;

	/*
	 * if targetlist include '*' we simple return
	 */
	foreach(lc, targetlist)
	{
		ResTarget  *res = (ResTarget *) lfirst(lc);

		if (IsA(res->val, ColumnRef))
		{
			ColumnRef  *cref = (ColumnRef *) res->val;

			if (IsA(llast(cref->fields), A_Star))
			{
				return;
			}
		}
		else if (IsA(res->val, A_Indirection))
		{
			A_Indirection *ind = (A_Indirection *) res->val;

			if (IsA(llast(ind->indirection), A_Star))
			{
				return;
			}
		}
	}

	context.targetlist = targetlist;

	foreach (lc, sortclause)
	{
		SortBy *sortby = (SortBy *) lfirst(lc);
		Node *node = sortby->node;

		if (IsA(node, A_Const))
		{
			Value	   *val = &((A_Const *) node)->val;
			int			location = ((A_Const *) node)->location;
			int			targetlist_pos = 0;
			int			target_pos;
			ListCell	*tl;
			bool 		found = false;

			if(IsA(val, Null)) {
				continue;
			}

			if (!IsA(val, Integer))
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
				/* translator: %s is name of a SQL construct, eg ORDER BY */
						 errmsg("non-integer constant in %s",
								ParseExprKindName(EXPR_KIND_ORDER_BY)),
						 parser_errposition(cur_pstate, location)));

			target_pos = intVal(val);
			foreach(tl, targetlist)
			{
				ResTarget  *res = (ResTarget *) lfirst(tl);

				if (++targetlist_pos == target_pos)
				{
					ColumnRef *cref;

					found = true;
					if (nodeTag(res->val) == T_ColumnRef)
						cref = (ColumnRef *) res->val;
					else if (nodeTag(res->val) == T_ColumnRefOrFuncCall)
					{
						ColumnRefOrFuncCall *cfcall = (ColumnRefOrFuncCall *) res->val;

						cref = (ColumnRef *) cfcall->cref;
					}
					else
						elog(ERROR, "order siblings by position doesn't support expr");

					sortby->node = (Node *) copyObject(cref);
					break;
				}
			}
			if (found)
				continue;
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_COLUMN_REFERENCE),
					 errmsg("%s position %d is not in select list",
							ParseExprKindName(EXPR_KIND_ORDER_BY), target_pos),
					 parser_errposition(cur_pstate, location)));
		}
		else
			sortby->node = parsetree_tree_walker(sortby->node,
						replace_orderby_aliasname_walker, &context);
	}
	return;
}


/*
 * replace orderby aliasname walker
 */
static Node*
replace_orderby_aliasname_walker(Node *node, void *context)
{
	Node *result = node;
	replace_orderby_aliasname_context *ctx = (replace_orderby_aliasname_context *) context;

	if (node == NULL)
		return result;

	switch (nodeTag(node))
	{
		case T_ColumnRef:
			{
				ColumnRef *r = (ColumnRef *) node;

				if (list_length(r->fields) == 1)
				{
					char *refname = strVal(linitial(r->fields));
					ListCell *lc;

					if (pg_strcasecmp(refname, "level") == 0)
						return result;

					/* check targetlist */
					foreach(lc, ctx->targetlist)
					{
						ResTarget  *res = (ResTarget *) lfirst(lc);

						if (res->name != NULL &&
							strcmp(res->name, refname) == 0)
						{
							ColumnRef *cref;

							if (nodeTag(res->val) == T_ColumnRef)
								cref = (ColumnRef *) res->val;
							else if (nodeTag(res->val) == T_ColumnRefOrFuncCall)
							{
								ColumnRefOrFuncCall *cfcall = (ColumnRefOrFuncCall *) res->val;

								cref = (ColumnRef *) cfcall->cref;
							}
							else
								elog(ERROR, "order siblings by doesn't support this expr");

							result = (Node *) copyObject(cref);
							break;
						}
					}
				}
			}
			break;
		case T_SysConnectPath:
			{
				SysConnectPath *syspath = (SysConnectPath *) node;

				syspath->colnode = parsetree_tree_walker(syspath->colnode, replace_orderby_aliasname_walker, context);
				break;
			}
		case T_ConnectByIsCycle:
			break;
		case T_ConnectByRoot:
			{
				ConnectByRoot *croot = (ConnectByRoot *) node;

				croot->colnode = parsetree_tree_walker(croot->colnode, replace_orderby_aliasname_walker, context);
				break;
			}
		case T_SelectStmt:
			break;
		case T_PriorAexpr:
			elog(ERROR, "Specified pseudocolumn or operator not allowed here");
			break;
		case T_PseudoColumnRef:
			break;
		default:
			elog(ERROR, "unrecognized node type: %d", (int) nodeTag(node));
			break;
	}

	return result;
}


/*
 * add table reference for prior_column
 */
static List *
append_table_columnlist(List *table_name, List *prior_column)
{
	ListCell *lc;

	foreach(lc, prior_column)
	{
		ColumnRef *ref = (ColumnRef *) lfirst(lc);
		char *field;
		bool found = false;
		ListCell *llc;

		if (list_length(ref->fields) <= 1)
			continue;

		field = NameListToString(ref->fields);
		foreach (llc, table_name)
		{
			ColumnRef *rref = (ColumnRef *) lfirst(llc);
			char	*fields1;

			fields1 = NameListToString(rref->fields);

			if (strcmp(fields1, field) == 0)
			{
				pfree(fields1);
				found = true;
				break;
			}
			pfree(fields1);
		}
		if (!found)
			table_name = lappend(table_name, ref);
		pfree(field);
	}

	return table_name;
}


/*
 * operators is return a bool
 */
static bool
is_bool_operator(List *operators)
{
	Value *v;
	char *operator_name;
	int i;
	bool ret = false;

	if (operators == NIL)
		return false;

	v = linitial(operators);
	Assert(nodeTag(v) == T_String);
	operator_name = strVal(v);

	for (i = 0; i < bool_operators_len; i++)
	{
		if (strcmp(operator_name, bool_operators[i]) == 0)
		{
			ret = true;
			break;
		}
	}
	return ret;
}



/*
 * split joinclause or whereclause from pnode
 * we recognize joinclause if a a_expr which include more than
 * two different tables'cloumn
 * we split like this strategy:
 * if we pass replace_join to false, then when we find a where_clause
 * and if it is in a AND_CLAUSE, we replace it to const true, if it is in a OR_CLAUSE
 * we replace it to a const false.
 * at the other hand, if we pass replace_join to true, then when we find a join_clause
 * and if it is in a AND_CLAUSE, we replace it ot a const true, if it is in a OR_CLAUSE
 * we replace it to a const false.
 * and the finaly, the result pnode is we want result.
 *
 */
static bool
split_join_where_clause_recurse(Node **pnode, split_join_where_clause_context *context)
{
	bool ret = false;
	Node *node = *pnode;

	if (node == NULL)
	{
		if (list_length(*context->table_indexs) >= 2)
			return true;
		return ret;
	}

	/* Guard against stack overflow due to overly complex expressions */
	check_stack_depth();

	switch (nodeTag(node))
	{
		case T_ResTarget:
			{
				ResTarget *res = (ResTarget *) node;

				ret = split_join_where_clause_recurse(&res->val, context);
				break;
			}
		case T_ColumnRef:
			{
				ColumnRef *r = (ColumnRef *) node;
				Node *tnode;

				/* handle outjoin */
				if (r->has_outer_join)
				{
					if (context->internel_outerjoin)
						*context->internel_outerjoin = true;
				}

				tnode = IvyTransformColumnRef(context->cur_pstate, r, true);

				/* is a table.col */
				if (tnode != NULL &&
					(nodeTag(tnode) == T_Var &&
					((Var *) tnode)->varlevelsup == 0))
				{
					ListCell *lc;
					bool diff = false;
					Var *colvar = (Var *) tnode;

					foreach (lc, *context->table_indexs)
					{
						Value *value = lfirst(lc);

						Assert(nodeTag(value) == T_Integer);

						/* compare oid */
						if (intVal(value) != colvar->varno)
						{
								diff = true;
								break;
						}
					}
					ret = diff;
					if (diff || list_length(*context->table_indexs) == 0)
					{
						*context->table_indexs = lappend(*context->table_indexs, makeInteger(colvar->varno));
						if (!context->twice_replace_outjoin &&
							context->outjoin_indexs &&
							*context->internel_outerjoin)
						{
							/* record outjoin table_index */
							*context->outjoin_indexs = lappend(*context->outjoin_indexs, makeInteger(colvar->varno));
						}
					}
					if (!ret &&
						context->twice_replace_outjoin &&
						context->outjoin_indexs &&
						*context->internel_outerjoin)
					{
						/*
						 * check it is a outjoin table of column
						 * oracle handle a.id(+) = b.id, then all outjoin about table a columns
						 * will consider join_clause not where clause even that a outjoin which has
						 * reference only a column like a.xx(+) = 23
						 */
						foreach (lc, *context->outjoin_indexs)
						{
							Value *value = lfirst(lc);

							Assert(nodeTag(value) == T_Integer);

							if (intVal(value) == colvar->varno)
							{
								diff = true;
								break;
							}
						}
						ret = diff;
					}
				}
				break;
			}
		case T_ParamRef:
		case T_OraParamRef:
			break;
		case T_A_Const:
			break;
		case T_PseudoColumnRef:
			break;

		case T_A_Indirection:
			{
				A_Indirection *ind = (A_Indirection *) node;

				ret = split_join_where_clause_recurse(&ind->arg, context);
				break;
			}

		case T_A_ArrayExpr:
			break;

		case T_TypeCast:
			ret = split_join_where_clause_recurse(&((TypeCast *) node)->arg, context);
			break;

		case T_CollateClause:
			ret = split_join_where_clause_recurse(&((CollateClause *) node)->arg,
									context);
			break;

		case T_A_Expr:
			{
				A_Expr	   *a = (A_Expr *) node;
				Node		*newnode;
				List		*newtable_indexs = NIL;
				bool		tmp_prior = false;
				bool		tmp_internel_prior1 = false;
				bool		tmp_internel_prior2 = false;
				List		*newoutjoin_indexs = NIL;
				bool		ret_temp;
				bool	*save_tmp_prior;
				List **save_tableindexs;
				bool *save_tmp_internel_prior1;
				List **save_new_outjoinindexs = NULL;
				int save_flags;

				/*
				 * Oracle handle between like than:
				 * h1.c1 betwwen h1.c1 + 1 and h2.c1 + 2
				 * which h1.c1 >= h1.c1 + 1 as where_clause
				 * h1.c1 <= h2.c1 + 2 as join_clause, so we should
				 * spit betwwen as a and_expr
				 */
				switch (a->kind)
				{
					case AEXPR_BETWEEN:
						{
							/* construct a and expr */
							Node *lexpr;
							Node *rexpr;
							Node *newlexpr;
							Node *newrexpr;

							if (nodeTag(a->rexpr) != T_List ||
								list_length((List *) a->rexpr) != 2)
								elog(ERROR, "invalid list in between and");

							lexpr = (Node *) linitial((List *) a->rexpr);
							rexpr = (Node *) lsecond((List *) a->rexpr);

							newlexpr = (Node *) makeA_Expr(AEXPR_OP, list_make1(makeString(pstrdup(">="))),
												a->lexpr, lexpr, -1);
							newrexpr = (Node *) makeA_Expr(AEXPR_OP, list_make1(makeString(pstrdup("<="))),
												copyObject(a->lexpr), rexpr, -1);
							*pnode = makeAndExpr(newlexpr, newrexpr, -1);
							save_flags = context->flags;
							context->flags = PRE_AND_OPT;
							ret =  split_join_where_clause_recurse(pnode, context);
							context->flags = save_flags;
							return ret;
							break;
						}
						break;
					case AEXPR_NOT_BETWEEN:
						{
							/* construct a or expr */
							Node *lexpr;
							Node *rexpr;
							Node *newlexpr;
							Node *newrexpr;

							if (nodeTag(a->rexpr) != T_List ||
								list_length((List *) a->rexpr) != 2)
								elog(ERROR, "invalid list in not between and");

							lexpr = (Node *) linitial((List *) a->rexpr);
							rexpr = (Node *) lsecond((List *) a->rexpr);

							newlexpr = (Node *) makeA_Expr(AEXPR_OP, list_make1(makeString(pstrdup("<"))),
												a->lexpr, lexpr, -1);
							newrexpr = (Node *) makeA_Expr(AEXPR_OP, list_make1(makeString(pstrdup(">"))),
												copyObject(a->lexpr), rexpr, -1);
							*pnode = makeOrExpr(newlexpr, newrexpr, -1);
							save_flags = PRE_AND_OPT;
							context->flags = PRE_OR_OPT;
							ret = split_join_where_clause_recurse( pnode, context);
							context->flags = save_flags;
							return ret;
							break;
						}
					case AEXPR_IN:
						{
							/* construct a and a_expr */
							Node *newexpr;
							Node *newrexpr;
							ListCell *lc;
							bool is_not_in = false;
							int	save_flags = context->flags;

							if (nodeTag(a->rexpr) != T_List)
								elog(ERROR, "invalid list in in_expr");

							lc = list_head((List *) a->rexpr);
							newrexpr = (Node *) lfirst(lc);
							newexpr = (Node *) makeA_Expr(AEXPR_OP, a->name,
												a->lexpr, newrexpr, -1);
							is_not_in = (strcmp(strVal(linitial(a->name)), "<>") == 0);

							/*
							 * it not in, we build and expr
							 * if in, we build or expr
							 */
							while ((lc = lnext((List *) a->rexpr, lc)) != NULL)
							{
								newrexpr = (Node *) lfirst(lc);
								newrexpr = (Node *) makeA_Expr(AEXPR_OP, a->name,
													copyObject(a->lexpr), newrexpr, -1);
								if (is_not_in)
								{
									newexpr = (Node *) makeAndExpr(newexpr, newrexpr, -1);
									context->flags = PRE_AND_OPT;
								}
								else
								{
									newexpr = (Node *) makeOrExpr(newexpr, newrexpr, -1);
									context->flags = PRE_OR_OPT;
								}
							}
							*pnode = newexpr;
							ret = split_join_where_clause_recurse(pnode, context);
							context->flags = save_flags;
							return ret;
							break;
						}
					default:
						break;
				}

				save_tableindexs = context->table_indexs;
				save_tmp_prior = context->has_prior;
				save_tmp_internel_prior1 = context->internel_outerjoin;

				if (!context->twice_replace_outjoin)
				{
					save_new_outjoinindexs = context->outjoin_indexs;

					context->outjoin_indexs = &newoutjoin_indexs;

				}

				context->table_indexs = &newtable_indexs;
				context->has_prior = &tmp_prior;
				context->internel_outerjoin = &tmp_internel_prior1;

				/*
				 * handle a.xx(+) = 23 as a joinclause,
				 * we should not replace the first ret value by the second
				 */
				ret = split_join_where_clause_recurse(&a->lexpr, context);

				context->internel_outerjoin = &tmp_internel_prior2;
				ret_temp = split_join_where_clause_recurse( &a->rexpr, context);
				ret = (ret || ret_temp);

				/* recover to value */
				context->table_indexs = save_tableindexs;
				context->has_prior = save_tmp_prior;
				context->internel_outerjoin = save_tmp_internel_prior1;

				if (!context->twice_replace_outjoin)
				{
					context->outjoin_indexs = save_new_outjoinindexs;

				}

				/*
				 * like '=','!=' we should replace others like
				 * '+', '-', we should not replace
				 */
				if (is_bool_operator(a->name))
				{
					if ((!context->has_outerjoin && ((ret && context->replace_join && !tmp_prior) ||
						(!ret && !context->replace_join))))
					{
						switch (context->flags)
						{
							case PRE_AND_OPT:
								/* construct a const true */
								newnode = makeBoolAConst(true);
								*pnode = newnode;
								list_free(newtable_indexs);
								break;
							case PRE_OR_OPT:
								/* construct a const false */
								newnode = makeBoolAConst(false);
								*pnode = newnode;
								list_free(newtable_indexs);
								break;
							default:
								*context->table_indexs = list_concat_unique(*context->table_indexs, newtable_indexs);
								if (list_length(*context->table_indexs) >= 2)
									ret = true;
								break;
						}

						if (tmp_internel_prior1 && tmp_internel_prior2)
							elog(ERROR, "a predicate may reference only one outer-joined table");
						if (!context->twice_replace_outjoin &&
							context->replace_join &&
							context->outjoin_indexs &&
							(tmp_internel_prior1 ||
							tmp_internel_prior2))
							*context->outjoin_indexs = list_concat_unique(*context->outjoin_indexs, newoutjoin_indexs);
						list_free(newoutjoin_indexs);

					}
				}
				else
				{
					*context->table_indexs = list_concat_unique(*context->table_indexs, newtable_indexs);
					if (list_length(*context->table_indexs) >= 2)
						ret = true;

					/* we only consider has prior condition */
					if (tmp_prior)
						*context->has_prior = tmp_prior;

					if (!context->twice_replace_outjoin &&
						context->replace_join &&
						context->outjoin_indexs &&
						(tmp_internel_prior1 ||
						tmp_internel_prior2))
					{
						*context->internel_outerjoin = (tmp_internel_prior1 || tmp_internel_prior2);
						*context->outjoin_indexs = list_concat_unique(*context->outjoin_indexs, newoutjoin_indexs);
					}
					list_free(newoutjoin_indexs);
					list_free(newtable_indexs);
				}
				break;
			}
		case T_List:
			{
				bool ret = false;
				ListCell *lc;
				List *tlist = (List *) node;

				foreach (lc, tlist)
				{
					Node *node = (Node *) lfirst(lc);
					bool ret_temp;

					ret_temp = split_join_where_clause_recurse(&node, context);
					lfirst(lc) = node;
					ret = (ret_temp || ret);
				}
				break;
			}

		case T_BoolExpr:
			{
				BoolExpr *bexpr = (BoolExpr *) node;
				int save_flags = context->flags;

				switch (bexpr->boolop)
				{
					case AND_EXPR:
						context->flags = PRE_AND_OPT;
						list_free(*context->table_indexs);
						*context->table_indexs = NIL;
						break;
					case OR_EXPR:
						context->flags = PRE_OR_OPT;
						list_free_deep(*context->table_indexs);
						*context->table_indexs = NIL;
						break;
					case NOT_EXPR:
						context->flags = PRE_OR_OPT;
						list_free(*context->table_indexs);
						*context->table_indexs = NIL;
						break;
					default:
						elog(ERROR, "doesn't recognize boolop %u", bexpr->boolop);
				}
				ret = split_join_where_clause_recurse((Node **) &bexpr->args, context);
				context->flags = save_flags;

				/*
				 * if where clause is hh.c1 = h1.c1 or (hh.c1 > 1 and h1.c2 = 2)
				 * when we have replaced hh.c1 > 1 and h1.c2 = 2 to true and true
				 * the join tree has become to hh.c1 = h1.c1 or (true and true),which
				 * we should replace it hh.c1 = h1.c1 and
				 * another situations hh.c1 = h1.c1 and (flase or flase), we should replace
				 * it to hh.c1 = h1.c1
				 * we shoud recursive this conditions, we only consider, the left or right
				 * arg is awlays  a const
				 *
				 */
				if (bexpr->boolop == AND_EXPR ||
					bexpr->boolop == OR_EXPR ||
					bexpr->boolop == NOT_EXPR)
				{
					ListCell *lc;
					List *newlist = NIL;

					foreach (lc, bexpr->args)
					{
						Node *tnode = (Node *) lfirst(lc);

						if (nodeTag(tnode) == T_TypeCast)
							tnode = ((TypeCast *) tnode)->arg;

						if (nodeTag(tnode) != T_A_Const)
							newlist = lappend(newlist, tnode);
					}
					if (newlist == NIL)
					{
						if (bexpr->boolop == AND_EXPR || bexpr->boolop == NOT_EXPR)
							*pnode = makeBoolAConst(true);
						else
							*pnode = makeBoolAConst(false);
					}
					else
						bexpr->args = newlist;
				}
				break;
			}

		case T_FuncCall:
			ret = split_join_where_clause_recurse((Node **) &((FuncCall *) node)->args, context);
			break;

		case T_ColumnRefOrFuncCall:
			ret = split_join_where_clause_recurse((Node **) &((ColumnRefOrFuncCall *) node)->cref,
													context);
			break;

		case T_MultiAssignRef:
			ret = split_join_where_clause_recurse(&((MultiAssignRef *) node)->source,
										context);
			break;

		case T_GroupingFunc:
			{
				GroupingFunc *gpf = (GroupingFunc *) node;
				bool	ret_temp1;
				bool	ret_temp2;

				/*
				 * handle a.xx(+) = 23 as a joinclause,
				 * we should not replace the first ret value by the second
				 */
				ret = split_join_where_clause_recurse((Node **) &gpf->args, context);
				ret_temp1 = split_join_where_clause_recurse((Node **) &gpf->refs, context);
				ret_temp2 = split_join_where_clause_recurse((Node **) &gpf->cols, context);
				ret = (ret || ret_temp1 || ret_temp2);
				break;
			}

		case T_NamedArgExpr:
			{
				NamedArgExpr *nexpr = (NamedArgExpr *) node;

				ret = split_join_where_clause_recurse((Node **) &nexpr->arg, context);
				break;
			}
		case T_SubLink:
			{
				/*
				 * SubLink is awalys considerd where_clause
				 */
				if (!context->replace_join)
				{
					if (context->flags == PRE_AND_OPT)
						*pnode = makeBoolAConst(true);
					else
						*pnode = makeBoolAConst(false);
				}
				list_free(*context->table_indexs);
				*context->table_indexs = NIL;
				break;
			}

		case T_CaseExpr:
			{
				bool	ret_temp1;
				bool	ret_temp2;

				/*
				 * handle a.xx(+) = 23 as a joinclause,
				 * we should not replace the first ret value by the second
				 */
				ret = split_join_where_clause_recurse((Node **) &((CaseExpr *) node)->arg,
												context);
				ret_temp1 = split_join_where_clause_recurse((Node **) & ((CaseExpr *) node)->args, context);
				ret_temp2 = split_join_where_clause_recurse((Node **) &((CaseExpr *) node)->defresult,
												context);
				ret = (ret || ret_temp1 || ret_temp2);
				break;
			}

		case T_RowExpr:
			ret = split_join_where_clause_recurse((Node **) &((RowExpr *) node)->args, context);
			break;

		case T_CoalesceExpr:
			ret = split_join_where_clause_recurse((Node **)  &((CoalesceExpr *) node)->args, context);
			break;

		case T_MinMaxExpr:
			ret = split_join_where_clause_recurse((Node **) &((MinMaxExpr *) node)->args, context);
			break;

		case T_XmlExpr:
			{
				XmlExpr *xml = (XmlExpr *) node;
				bool	ret_temp1;
				bool	ret_temp2;

				/*
				 * handle a.xx(+) = 23 as a joinclause,
				 * we should not replace the first ret value by the second
				 */
				ret = split_join_where_clause_recurse((Node **)  &xml->arg_names, context);
				ret_temp1 = split_join_where_clause_recurse((Node **) &xml->named_args, context);
				ret_temp2 = split_join_where_clause_recurse((Node **) &xml->args, context);
				ret = (ret || ret_temp1 || ret_temp2);
				break;
			}

		case T_XmlSerialize:
			{
				XmlSerialize *xmls = (XmlSerialize *) node;

				ret = split_join_where_clause_recurse(&xmls->expr, context);
				break;
			}

		case T_NullTest:
			{
				NullTest   *n = (NullTest *) node;
				bool		tmp_prior = false;
				List *new_tableindexs = NIL;
				List *new_outjoinindexs = NIL;
				bool tmp_internel_prior1 = false;
				bool	*save_tmp_prior;
				List **save_tableindexs;
				bool *save_tmp_internel_prior1;
				List **save_new_outjoinindexs = NULL;

				save_tableindexs = context->table_indexs;
				save_tmp_prior = context->has_prior;
				save_tmp_internel_prior1 = context->internel_outerjoin;
				if (!context->twice_replace_outjoin)
				{
					save_new_outjoinindexs = context->outjoin_indexs;

					context->outjoin_indexs = &new_outjoinindexs;

				}

				context->table_indexs = &new_tableindexs;
				context->has_prior = &tmp_prior;
				context->internel_outerjoin = &tmp_internel_prior1;

				ret = split_join_where_clause_recurse((Node **) &n->arg, context);

				/* recover to value */
				context->table_indexs = save_tableindexs;
				context->has_prior = save_tmp_prior;
				context->internel_outerjoin = save_tmp_internel_prior1;

				if (!context->twice_replace_outjoin)
				{
					context->outjoin_indexs = save_new_outjoinindexs;

				}

				if ((!context->has_outerjoin && ((ret && context->replace_join && !tmp_prior) ||
					(!ret && !context->replace_join))))
				{
					*pnode = makeBoolAConst(true);

					if (!context->twice_replace_outjoin &&
						context->replace_join &&
						context->outjoin_indexs &&
						tmp_internel_prior1)
					{
						*context->outjoin_indexs = list_concat_unique(*context->outjoin_indexs, new_outjoinindexs);
					}
				}
				else if (tmp_prior)
					*context->has_prior = tmp_prior;
				list_free(new_tableindexs);
				list_free(new_outjoinindexs);
				break;
			}

		case T_BooleanTest:
			{
				BooleanTest *n = (BooleanTest *) node;
				bool		tmp_prior = false;
				List *new_tableindexs = NIL;
				List *new_outjoinindexs = NIL;
				bool tmp_internel_prior1 = false;
				bool	*save_tmp_prior;
				List **save_tableindexs;
				bool *save_tmp_internel_prior1;
				List **save_new_outjoinindexs = NULL;

				save_tableindexs = context->table_indexs;
				save_tmp_prior = context->has_prior;
				save_tmp_internel_prior1 = context->internel_outerjoin;
				if (!context->twice_replace_outjoin)
				{
					save_new_outjoinindexs = context->outjoin_indexs;

					context->outjoin_indexs = &new_outjoinindexs;

				}

				context->table_indexs = &new_tableindexs;
				context->has_prior = &tmp_prior;
				context->internel_outerjoin = &tmp_internel_prior1;

				ret = split_join_where_clause_recurse((Node **) &n->arg, context);

				/* recover to value */
				context->table_indexs = save_tableindexs;
				context->has_prior = save_tmp_prior;
				context->internel_outerjoin = save_tmp_internel_prior1;

				if (!context->twice_replace_outjoin)
				{
					context->outjoin_indexs = save_new_outjoinindexs;

				}

				if ((ret && context->replace_join && !tmp_prior) ||
						(!ret && !context->replace_join))
				{
					*pnode = makeBoolAConst(true);
					if (!context->twice_replace_outjoin &&
						context->replace_join &&
						context->outjoin_indexs &&
						tmp_internel_prior1)
					{
						*context->outjoin_indexs = list_concat_unique(*context->outjoin_indexs, new_outjoinindexs);
					}
				}
				else if (tmp_prior)
					*context->has_prior = tmp_prior;

				list_free(new_tableindexs);
				list_free(new_outjoinindexs);
			}

			break;

		case T_CurrentOfExpr:
			break;
		case T_CaseTestExpr:
		case T_SetToDefault:
		case T_Var:
		case T_Const:	/* BUGID:504 seq.nextval and seq.currval */
		case T_SysConnectPath:
		case T_ConnectByIsCycle:
		case T_ConnectByRoot:
			break;
		case T_SortBy:
			{
				SortBy *sb = (SortBy *) node;

				ret = split_join_where_clause_recurse(&sb->node, context);
				break;
			}
		case T_CaseWhen:
			{
				CaseWhen *cs = (CaseWhen *) node;
				bool	ret_temp1;
				bool	ret_temp2;

				/*
				 * handle a.xx(+) = 23 as a joinclause,
				 * we should not replace the first ret value by the second
				 */

				ret = split_join_where_clause_recurse((Node **) &cs->expr, context);
				ret_temp1 = split_join_where_clause_recurse((Node **) &cs->orig_expr, context);
				ret_temp2 = split_join_where_clause_recurse((Node **) &cs->result, context);
				ret = (ret || ret_temp1 || ret_temp2);
				break;
			}
		case T_PriorAexpr:
			/* prior operator is awalys a where expr */
			if (!context->replace_join)
			{
				Node *newnode;

				switch (context->flags)
				{
					case PRE_AND_OPT:
						/* construct a const true */
						newnode = makeBoolAConst(true);
						*pnode = newnode;
						break;
					case PRE_OR_OPT:
						/* construct a const false */
						newnode = makeIntConst(false);
						*pnode = newnode;
						break;
					default:
						break;
				}
			}
			ret = false;
			*context->has_prior = true;
			break;
		/* Begin - BUG#7712 */
		case T_OraRowExpr:
			ret = split_join_where_clause_recurse((Node **) &((OraRowExpr *) node)->args, context);
			break;
		/* End - BUG#7712 */
		default:
			break;
	}

	/* maybe table_indexs has more than two tables */
	if (list_length(*context->table_indexs) >= 2)
		return true;
	return ret;
}


/*
 * remove outjoin properity
 */
static Node *
remove_outjoin_walker(Node *node, void *context)
{
	Node *result = node;

	if (node == NULL)
		return result;

	switch (nodeTag(node))
	{
		case T_ColumnRef:
			{
				ColumnRef *cref = (ColumnRef *) node;

				cref->has_outer_join = false;
				break;
			}
		case T_SysConnectPath:
			{
				SysConnectPath *syspath = (SysConnectPath *) node;

				syspath->colnode = parsetree_tree_walker(syspath->colnode, remove_outjoin_walker, context);
				break;
			}
		case T_ConnectByIsCycle:
			break;
		case T_ConnectByRoot:
			{
				ConnectByRoot *croot = (ConnectByRoot *) node;

				croot->colnode = parsetree_tree_walker(croot->colnode, remove_outjoin_walker, context);
				break;
			}
		case T_SelectStmt:
		case T_PriorAexpr:
		case T_PseudoColumnRef:
			break;
		default:
			elog(ERROR, "unrecognized node type: %d", (int) nodeTag(node));
			break;
	}

	return result;
}



/*
 * replace series walker
 */
static Node *
replace_series_walker(Node *node, void *context)
{
	Node *result = node;
	replace_series_context *ctx = (replace_series_context *) context;

	if (node == NULL)
		return result;

	switch (nodeTag(node))
	{
		case T_ColumnRef:
			{
				ColumnRef *r = (ColumnRef *) node;
				Node *strnode;

				strnode = linitial(r->fields) ;

				if (ctx->pre_prior && ctx->prior_columlist != NULL)
					*ctx->prior_columlist = lappend(*ctx->prior_columlist, copyObject(r));

				if (list_length(r->fields) == 1 &&
					T_String == nodeTag(strnode) &&
					pg_strcasecmp(strVal(strnode), "level") == 0)
				{
					/* replace level to level + 1 */
					Node *anode;

					/*
					 * if we in a subquery
					 *   and it has a connect by, we
					 *   should not replace level and no handle
					 *   if not has a connectby,we should raise error
					 *   or we will use the parent connect by'level
					 *   which is wrong
					 */
					if (ctx->child_pstate != NULL)
					{
						if (ctx->child_hasconnectby)
							break;
						else
							elog(ERROR, "CONNECT BY clause required in this query block");
					}

					if (ctx->pre_prior)
						elog(ERROR, "PRIOR doesn't be allowed to use level");

					if (*ctx->recurse_flag == CONNECT_BY_REPLACE_LEVEL)
					{
						anode = makeIntConst(0);
					}
					else
					{
						anode = (Node *) makeSimpleA_Expr(AEXPR_OP, "+",
														node,
												(Node *) makeIntConst(1),
											-1);
						*ctx->recurse_flag |= CONNECT_BY_CONTAIN_LEVEL;
					}
					result = (Node *) anode;
				}
				else if (*ctx->recurse_flag != CONNECT_BY_REPLACE_LEVEL)
				{
					Node *filednode;
					bool	connectby_col = false;

					/* ignore * */
					if (IsA(llast(r->fields), A_Star))
						connectby_col = true;
					else
					{
						if (ctx->child_pstate != NULL)
						{
							Node *tnode;
							ColumnRef *tmpref = copyObject(r);

							tnode = IvyTransformColumnRef(ctx->child_pstate, tmpref, true);

							/* Begin - bug#00008247 */
							if (tnode == NULL ||
								nodeTag(tnode) != T_Var ||
								((Var *) tnode)->varlevelsup == ctx->sublevel)
								connectby_col = true;
							/* End - bug#00008247 */
							if (tnode != NULL)
								pfree(tnode);
							pfree(tmpref);
						}
						else
							connectby_col = true;

						if (connectby_col && ctx->cur_pstate != NULL)
						{
							/* maybe out level col or variable */
							Node *tnode;
							ColumnRef *tmpref = copyObject(r);

							tnode = IvyTransformColumnRef(ctx->cur_pstate, tmpref, true);
							if (tnode != NULL  &&
								(nodeTag(tnode) != T_Var ||
								((Var *) tnode)->varlevelsup != 0))
								connectby_col = false;
							if (tnode)
								pfree(tnode);
							pfree(tmpref);
						}
					}
					if (connectby_col)
					{
						switch (list_length(r->fields))
						{
							case 1:
								r->fields = lcons(makeString(pstrdup(ctx->pre_prior ? ctx->prior_name : ctx->tbname)), r->fields);
								break;
							case 2:
								filednode = linitial(r->fields);
								strVal(filednode) = ctx->pre_prior ? ctx->prior_name : ctx->tbname;
								break;
							case 3:
								filednode = lsecond(r->fields);
								strVal(filednode) = ctx->pre_prior ? ctx->prior_name : ctx->tbname;
								/* remove schema */
								r->fields = list_delete_first(r->fields);
								break;
							default:
								filednode = (Node *) lfirst(list_nth_cell(r->fields,
									list_length(r->fields) - 2));
								strVal(filednode) = ctx->pre_prior ? ctx->prior_name : ctx->tbname;
								/* remove schema */
								r->fields = list_delete_first(r->fields);
								break;
						}
					}
				}
			}
			break;
		case T_SysConnectPath:
		case T_ConnectByIsCycle:
		case T_ConnectByRoot:
			break;
		case T_SelectStmt:
			{
				SelectStmt *stmt = (SelectStmt *) node;
				ParseState *newpstate;
				bool	child_has_connectby = (stmt->connect_by != NULL);
				bool	saved_child_hasconnectby = ctx->child_hasconnectby;
				ParseState *saved_child_pstate = ctx->child_pstate;
				/* Begin - BUG#Z00005447 */
				List *from_clause;
				/* End - BUG#Z00005447 */

				newpstate = make_parsestate(ctx->child_pstate == NULL ? ctx->cur_pstate : ctx->child_pstate); /* bug#00008247 */
				newpstate->p_sourcetext = NULL;

				/* Begin - BUG#Z00005447 */
				/*
				 * there, transformFromClause maybe change origin
				 * fromclauseitem like JoinExpr, so used in later
				 * mabye report error, so we copy it
				 */
				from_clause = copyObject(stmt->fromClause);
				transformFromClause(newpstate, from_clause);
				list_free(from_clause);
				/* End - BUG#Z00005447 */
				newpstate->p_expr_kind = EXPR_KIND_SELECT_TARGET;

				ctx->child_hasconnectby = child_has_connectby;
				ctx->child_pstate = newpstate;
				ctx->sublevel++; /* bug#00008247 */

				stmt->targetList = (List *) parsetree_tree_walker((Node *) stmt->targetList,
								replace_series_walker, context);

				newpstate->p_expr_kind = EXPR_KIND_WHERE;
				stmt->whereClause= parsetree_tree_walker(stmt->whereClause,
								replace_series_walker, context);

				newpstate->p_expr_kind = EXPR_KIND_GROUP_BY;
				stmt->sortClause = (List *) parsetree_tree_walker((Node *) stmt->sortClause,
									replace_series_walker, context);

				newpstate->p_expr_kind = EXPR_KIND_LIMIT;
				stmt->limitOffset = parsetree_tree_walker(stmt->limitOffset,
										replace_series_walker, context);
				stmt->limitCount = parsetree_tree_walker(stmt->limitCount,
										replace_series_walker, context);
				stmt->lockingClause = (List *) parsetree_tree_walker((Node *) stmt->lockingClause,
								replace_series_walker, context);

				newpstate->p_expr_kind = EXPR_KIND_GROUP_BY;
				stmt->groupClause = (List *) parsetree_tree_walker((Node *) stmt->groupClause,
									replace_series_walker, context);

				newpstate->p_expr_kind = EXPR_KIND_HAVING;
				stmt->havingClause = parsetree_tree_walker(stmt->havingClause,
							replace_series_walker, context);
				free_parsestate(newpstate);

				ctx->child_hasconnectby = saved_child_hasconnectby;
				ctx->child_pstate = saved_child_pstate;
				ctx->sublevel--; /* bug#00008247 */
			}
			break;
		case T_PseudoColumnRef:
			/* replace rownum to level */
			if (ctx->pre_prior)
				elog(ERROR, "PRIOR doesn't be allowed to use ROWNUM");
			if (*ctx->recurse_flag != CONNECT_BY_REPLACE_LEVEL &&
				((PseudoColumnRef *) node)->pckind == PSEUDOCOL_KIND_ROWNUM)
			{
				result = makeColumnRef(pstrdup("level"), NIL);
				*ctx->recurse_flag |= CONNECT_BY_CONTAIN_ROWNUM;
			}
			break;
		case T_PriorAexpr:
			{
				PriorAexpr *paexpr = (PriorAexpr *) node;

				if (ctx->pre_prior)
					elog(ERROR, "Specified pseudocolumn or operator not allowed here");

				ctx->pre_prior = true;
				paexpr->arg = parsetree_tree_walker(paexpr->arg, replace_series_walker, context);

				ctx->pre_prior = false;

				result = paexpr->arg;
				*ctx->found_prior = true;
			}
			break;
		default:
			elog(ERROR, "unrecognized node type: %d", (int) nodeTag(node));
			break;
	}

	return result;
}



/*
 * replace rownum to $1
 */
static Node*
replace_rownum_to_param_walker(Node *node, void *context)
{
	Node *result = node;

	if (node == NULL)
		return result;

	switch (nodeTag(node))
	{
		case T_ColumnRef:
			break;
		case T_SysConnectPath:
			{
				SysConnectPath *syspath = (SysConnectPath *) node;

				syspath->colnode = parsetree_tree_walker(syspath->colnode, replace_rownum_to_param_walker, context);
				break;
			}
		case T_ConnectByIsCycle:
			break;
		case T_ConnectByRoot:
			{
				ConnectByRoot *croot = (ConnectByRoot *) node;

				croot->colnode = parsetree_tree_walker(croot->colnode, replace_rownum_to_param_walker, context);
				break;
			}
		case T_SelectStmt:
			break;
		case T_PriorAexpr:
			((PriorAexpr *) node)->arg = parsetree_tree_walker(((PriorAexpr *) node)->arg, replace_rownum_to_param_walker, context);
			result = ((PriorAexpr *) node)->arg;
			break;
		case T_PseudoColumnRef:
			/* replace rownum to $1 */
			if (((PseudoColumnRef *) node)->pckind == PSEUDOCOL_KIND_ROWNUM)
			{
				ParamRef *pref = makeNode(ParamRef);

				pref->number = 1;
				pref->location = -1;
				result = (Node *) pref;
			}
			break;
		default:
			elog(ERROR, "unrecognized node type: %d", (int) nodeTag(node));
			break;
	}

	return result;
}



/*
 * replace priorcolname recurse
 */
static Node *
replace_priorcolname_walker(Node *node, void *context)
{
	Node *result = node;
	replace_priorcolname_context *ctx = (replace_priorcolname_context *) context;

	if (node == NULL)
		return result;

	switch (nodeTag(node))
	{
		case T_ColumnRef:
			if (ctx->found_prior)
			{
				ColumnRef *r = (ColumnRef *) node;
				Node *nref;
				char *refname;
				StringInfoData infodata;
				ListCell *lc;
				bool found = false;

				refname = NameListToString(r->fields);

				if (pg_strcasecmp(refname, "level") == 0)
					elog(ERROR, "PRIOR doesn't be allowed to use level");

				initStringInfo(&infodata);
				appendStringInfo(&infodata, "%s%s", ctx->prior_colname_prefix, refname);
				nref = makeColumnRef(infodata.data, NIL);

				/* has already append */
				foreach (lc, *ctx->resultlist)
				{
					ColumnRef *lr = (ColumnRef *) lfirst(lc);
					char *namestr = NULL;

					Assert(lr->type == T_ColumnRef);

					namestr = NameListToString(lr->fields);
					if (pg_strcasecmp(namestr, refname) == 0)
					{
						found = true;
						pfree(namestr);
						break;
					}
					pfree(namestr);
				}
				if (!found)
					*ctx->resultlist = lappend(*ctx->resultlist, node);
				result = nref;
				pfree(refname);
			}
			break;
		case T_SysConnectPath:
		case T_ConnectByIsCycle:
		case T_ConnectByRoot:
		case T_SelectStmt:
			break;
		case T_PriorAexpr:
			{
				CaseExpr *case_expr;
				CaseWhen *w;
				A_Const *anull;
				PriorAexpr *priorexpr = (PriorAexpr *) node;

				if (ctx->found_prior)
					elog(ERROR, "Specified pseudocolumn or operator not allowed here");

				ctx->found_prior = true;
				priorexpr->arg = parsetree_tree_walker(priorexpr->arg,  replace_priorcolname_walker, ctx);

				ctx->found_prior = false;

				case_expr = makeNode(CaseExpr);
				case_expr->casetype = InvalidOid;
				case_expr->defresult = (Expr *) ((PriorAexpr *) node)->arg;
				case_expr->arg = (Expr *) makeColumnRef("level", NIL);
				w = makeNode(CaseWhen);
				w->expr = (Expr *) makeIntConst(1);
				anull =  makeNode(A_Const);
				anull->val.type = T_Null;
				anull->location = -1;
				w->result = (Expr *) anull;
				case_expr->args = list_make1(w);

				result = (Node *) case_expr;
				/* rely on level */
				*ctx->flags |= CONNECT_BY_CONTAIN_LEVEL;

				break;
			}
		case T_PseudoColumnRef:
			if (ctx->found_prior)
				elog(ERROR, "PRIOR doesn't be allowed to use ROWNUM");
			break;
		default:
			elog(ERROR, "unrecognized node type: %d", (int) nodeTag(node));
			break;
	}

	return result;
}



/*
 * use transformColumnRef to get column oid
 */
static Oid
getColumnTypeidFromColumnref(ParseState *cur_pstate, ColumnRef *ref,
											int32 *colmod,
											Oid *colcollaction)
{
	Oid typid;
	ColumnRef *tmpref;
	Node *colnode;
	ParseExprKind saved_kind = cur_pstate->p_expr_kind;

	tmpref = copyObject(ref);

	cur_pstate->p_expr_kind = EXPR_KIND_SELECT_TARGET;

	colnode = transformExpr(cur_pstate, (Node *) tmpref, EXPR_KIND_SELECT_TARGET);
	typid = exprType(colnode);
	*colmod = exprTypmod(colnode);
	*colcollaction = exprCollation(colnode);

	cur_pstate->p_expr_kind = saved_kind;

	pfree(tmpref);
	pfree(colnode);

	return typid;
}



/*
 * expand table.*
 */
static List *
expandTargetListAstar(ParseState *cur_pstate, List *oldtargets)
{
	ListCell *lc;
	List *retlist = NIL;

	foreach (lc, oldtargets)
	{
		ResTarget *res = (ResTarget *) lfirst(lc);

		/* only consider columnref */
		switch (nodeTag(res->val))
		{
			case T_ColumnRef:
				{
					ColumnRef *cref = (ColumnRef *) res->val;
					List *fields = cref->fields;
					int numnames = list_length(fields);
					char *relname = NULL;
					char *nspname = NULL;
					int levels_up;
					ParseNamespaceItem *nsitem = NULL;

					if (numnames > 1 &&
						IsA(llast(cref->fields), A_Star))
					{
						switch (numnames)
						{
							case 2:
								relname = strVal(linitial(fields));
								nsitem = refnameNamespaceItem(cur_pstate, nspname, relname,
														   cref->location,
														   &levels_up);
								break;
							case 3:
								nspname = strVal(linitial(fields));
								relname = strVal(lsecond(fields));
								nsitem = refnameNamespaceItem(cur_pstate, nspname, relname,
														   cref->location,
														   &levels_up);
								break;
							case 4:
								{
									char	   *catname = strVal(linitial(fields));

									/*
									 * We check the catalog name and then ignore it.
									 */
									if (strcmp(catname, get_database_name(MyDatabaseId)) != 0)
									{
										break;
									}
									nspname = strVal(lsecond(fields));
									relname = strVal(lthird(fields));
									nsitem = refnameNamespaceItem(cur_pstate, nspname, relname,
															   cref->location,
															   &levels_up);
									break;
								}
							default:
								break;
						}
					}
					if (nsitem == NULL)
						retlist = lappend(retlist, res);
					else
					{
						List	   *names;
						ListCell *lc;


						expandRTE(nsitem->p_rte, nsitem->p_rtindex, levels_up, -1, false,
								  &names, NULL);
						foreach (lc, names)
						{
							char	   *label = strVal(lfirst(lc));
							ResTarget *newres = makeNode(ResTarget);
							ColumnRef *newref = copyObject(cref);

							lfirst(list_tail(newref->fields)) = makeString(pstrdup(label));
							newres->val = (Node *) newref;
							newres->location = -1;
							newres->name = pstrdup(label);
							retlist = lappend(retlist, newres);
						}
					}
				break;
			}
			default:
				retlist = lappend(retlist, res);
				break;
		}
	}

	return retlist;
}



/*
 * get collation name include schema'name
 */
static List*
get_collation_name_forlist(Oid colloid)
{
	HeapTuple	tp;

	tp = SearchSysCache1(COLLOID, ObjectIdGetDatum(colloid));
	if (HeapTupleIsValid(tp))
	{
		Form_pg_collation colltup = (Form_pg_collation) GETSTRUCT(tp);
		char	   *result;
		char		*spacename;

		result = pstrdup(NameStr(colltup->collname));

		spacename = get_namespace_name(colltup->collnamespace);
		Assert(spacename != NULL);
		ReleaseSysCache(tp);
		return list_make2(makeString(spacename), makeString(result));
	}
	else
		elog(ERROR, "invalid collation oid %u", colloid);

	return NIL;
}



/*
 * Constructs a CTE-level query named level__query to
 * replace connect by
 *
 * note:
 * connect by rownum < xxx we replace it to select * from (select
 * * from xxx connect by level < xxx -1) where rownum < xxx or level = 1
 *
 * connect by rownum xxx oper yyy, if xxx and yyy not reference prior operator and
 * connect by output cols doens't has parent cols like connect_by_isleaf,connect_by_iscycle
 * which should use parent rows to construct
 * we replace it as:
 * select * from (select * from _connect_by_rownum_query ax(xxx,yyy))
 * if we use level like origin,maybe level is biggest,so that results disk full error
 * another order by deep,if we use array(rownum) maybe it will results disk full error
 * becuase it deep level is more then 100k,which array is much big
 */
SelectStmt*
transformHierarQueryClause(ParseState *pstate, SelectStmt *stmt,
		IgnoreStr **ignoreStr, const char *source_sql)
{
	SelectStmt *finalStmt, *lStmt, *rStmt, *unionstmt, *tailstmt;
	SelectStmt *internel_select;
	RangeSubselect *finalSubselect = NULL, *finalSubselect2 = NULL;
	SelectStmt *allfinalstmt;
	Alias *finalAlias = NULL;
	Node *whereClause = NULL;
	HierarQueryClause *lq = NULL;
	char *tbname = NULL;
	Node *node = NULL;
	Node *unionnode = NULL;

	/* cte_0_array__path node */
	Node *nodepath1 = NULL;
	TypeCast*nodecastpath1 = NULL;
	Node *nodepath2 = NULL;
	TypeCast *nodecastpath2 = NULL;

	/*cycle_check node*/
	Node *rnode = NULL;
	Node *rsimplenode = NULL;
	TypeCast *rnodecast = NULL;
	Node *nocyclenode = NULL;
	CaseWhen *casewheniscycle = NULL;

	/* connect_by_isleaf node */
	CaseExpr *caseisleaf = NULL;
	CaseWhen *casewhenisleaf = NULL;
	List *caseisleaf_args = NIL;
	FuncCall *leadfun = NULL;

	ResTarget *res = NULL;
	ResTarget *level1 = NULL;
	ResTarget *isleaf = NULL;
	ResTarget *arrayres = NULL;

	bool haslevel = false;	/* has level */
	bool hasisleaf = false; /* has connect_by_isleaf */
	bool hascycle = false;	/* has connect_by_cycle */
	A_ArrayExpr *array1 = NULL;
	A_ArrayExpr *arraypath = NULL;

	ResTarget *targetnode = NULL;
	char *finalname = pstrdup("final_level__query");
	char *priorname = pstrdup("level__query");			/* CTE name */
	char *internel_priorname = pstrdup("level__query1");
	char *deeporderbycol = pstrdup("cte_0_by_order_col");	/* ignore alias name */
	char *sysconnectpath = pstrdup("sys_connect_by_path");
	char *allfinalname = pstrdup("allfinal_level__query");
	char *iscyclename = pstrdup("connect_by_iscycle");
	char *rootname = pstrdup("connect_by_root");
	char *levelcol = pstrdup("level");
	char *isleafcol = pstrdup("connect_by_isleaf");
	char *ctepath = pstrdup("cte_0_array__path");
	char *prior_colname_prefix = pstrdup("_prior");
	char *tb_alias = "_tbname_";
	bool origin_tb_alias = true; /* BUG#M0000326 */
	List *prior_colname_list = NIL;
	bool  connect_by_prior = false;
	List *connect_by_priorlist = NIL;
	char *prefix_table = "_col";
	ResTarget *colres;
	ColumnRef *colall;
	bool more_tables = false;
	List *table_column_list = NIL;
	int connect_by_flag = 0;
	Node *origin_connect_by = NULL;
	Node *connect_by_rownum = NULL;
	RangeVar * rv = NULL;
	int level_replace_flag = CONNECT_BY_REPLACE_LEVEL;
	bool prior_found;
	int psecol_flags = 0;
	List *rootlist = NIL;
	List *syspathlist = NIL;

	WithClause *tailwithclause = makeNode(WithClause);
	WithClause *internel_with = makeNode(WithClause);
	CommonTableExpr *cte = makeNode(CommonTableExpr);
	CommonTableExpr *internel_cte = makeNode(CommonTableExpr);
	List *tailfromclause = NIL;
	List *oldtarget = NIL;
	List *allfinaltarget = NIL;
	ColumnRef *all = NULL;
	FuncCall *rowfun = NULL;
	FuncCall *arrayfun = NULL;
	FuncCall *arraypathfun = NULL;
	FuncCall *rwherefunc = NULL;
	bool has_no_special_cols = true;
	List *base_colnames = NIL;
	RangeFunction *rangefunc = NULL;
	char *connect_by_rownum_func_str = "_connect_by_rownum_query";
	Node *origin_start_with = NULL;

	SortBy *sortnode = NULL;
	WithClause	    *withclause = NULL;
	List	   *groupClause = NIL;
	Node	   *havingClause = NULL;
	List	   *windowClause = NIL;
	List	   *sortClause = NIL;
	List	   *lockingClause = NIL;
	Node	   *limitOffset = NULL;
	Node	   *limitCount = NULL;
	Node	   *rwherenode = NULL;
	bool		siblings_by = false;
	ParseState	*cur_pstate = make_parsestate(pstate);
	replace_pseudocol_context pseudocol_ctx;
	add_tabreference_context tab_ctx;
	replace_priorcolname_context replace_prior_ctx;
	replace_series_context replace_series_ctx;
	remove_tabreference_context remove_tab_ctx;
	/* Begin - bug#00008247 */
	List	   *distinctClause = NIL;
	IntoClause *intoClause = NULL;
	/* End - bug#00008247 */

	/* save related field name to ignoreStr */
	*ignoreStr = (IgnoreStr*)palloc0(sizeof (IgnoreStr));
	(*ignoreStr)->lq_table = deeporderbycol;
	(*ignoreStr)->lq_level = levelcol;
	(*ignoreStr)->lq_isleaf = isleafcol;
	(*ignoreStr)->lq_arrpath = ctepath;

	/* get level query clause */
	lq = stmt->connect_by;
	stmt->connect_by = NULL;

	/* move start with condition to whereClause */
	if (stmt->whereClause)
	{
		whereClause = stmt->whereClause;
		stmt->whereClause = lq->start_with;
	}
	else
	{
		stmt->whereClause = lq->start_with;
	}

	/* init some contexts */
	pseudocol_ctx.connect_by_iscycle = iscyclename;
	pseudocol_ctx.connect_by_isleaf = isleafcol;
	pseudocol_ctx.connect_by_path = sysconnectpath;
	pseudocol_ctx.connect_by_root = rootname;
	pseudocol_ctx.finalname = allfinalname;
	pseudocol_ctx.flags = &psecol_flags;
	pseudocol_ctx.levelcol = levelcol;
	pseudocol_ctx.path_list = &syspathlist;
	pseudocol_ctx.rootlist = &rootlist;

	tab_ctx.child_hasconnectby = false;
	tab_ctx.child_pstate = NULL;
	tab_ctx.remove_schema = true;
	/* Begin - BUG#Z00005447 */
	tab_ctx.only_replace = false;
	/* End - BUG#Z00005447 */
	tab_ctx.cur_pstate = cur_pstate;
	tab_ctx.tbname = internel_priorname;
	/* Begin - bug#00008247 */
	tab_ctx.sublevel = 0;
	/* End - bug#00008247 */

	replace_prior_ctx.flags = &psecol_flags;
	replace_prior_ctx.resultlist = &prior_colname_list;
	replace_prior_ctx.prior_colname_prefix = prior_colname_prefix;
	replace_prior_ctx.found_prior = false;

	replace_series_ctx.child_hasconnectby = false;
	replace_series_ctx.child_pstate = NULL;
	replace_series_ctx.prior_columlist = NULL;
	replace_series_ctx.prior_name = "_prior";
	replace_series_ctx.tbname = "test";
	replace_series_ctx.pre_prior = false;
	replace_series_ctx.found_prior = &prior_found;
	replace_series_ctx.recurse_flag = &level_replace_flag;
	replace_series_ctx.cur_pstate = cur_pstate;
	replace_series_ctx.sublevel = 0; /* bug#00008247 */

	remove_tab_ctx.child_pstate = NULL;
	remove_tab_ctx.prefix_str = prefix_table;
	remove_tab_ctx.columnlist = &table_column_list;
	remove_tab_ctx.tbname = allfinalname;
	remove_tab_ctx.cur_pstate = cur_pstate;
	/* Begin - bug#00008247 */
	remove_tab_ctx.only_replace = false;
	remove_tab_ctx.sublevel = 0;
	/* End - bug#00008247 */

	origin_start_with = copyObject(lq->start_with);

	if (lq->start_with != NULL)
	{
		int locaiton = exprLocation(lq->start_with);

		lq->start_with = parsetree_tree_walker(lq->start_with,
						replace_priorcolname_walker, &replace_prior_ctx);

		/*
		 * doesn't allow prior column:ORA-00976
		 */
		if (psecol_flags != 0)
		   ereport(ERROR,
				   (errcode(ERRCODE_SYNTAX_ERROR),
					errmsg("Specified pseudocolumn or operator not allowed here"),
					parser_errposition(pstate, locaiton)));
	}

	/* get clause from stmt and make stmt clean*/
	{
		withclause = stmt->withClause;
		stmt->withClause = NULL;
		limitOffset = stmt->limitOffset;
		stmt->limitOffset = NULL;
		limitCount = stmt->limitCount;
		stmt->limitCount = NULL;
		lockingClause = stmt->lockingClause;
		stmt->lockingClause = NIL;
		sortClause = stmt->sortClause;
		stmt->sortClause = NIL;
		groupClause = stmt->groupClause;
		stmt->groupClause = NIL;
		havingClause = stmt->havingClause;
		stmt->havingClause = NULL;
		windowClause = stmt->windowClause;
		stmt->windowClause = NIL;
		/* Begin - bug#00008247 */
		distinctClause = stmt->distinctClause;
		stmt->distinctClause = NULL;
		intoClause = stmt->intoClause;
		stmt->intoClause = NULL;
		/* End - bug#00008247 */
	}


	/* process the FROM clause */
	/* check grammer */
	transformFromClause(cur_pstate, copyObject(stmt->fromClause));
	if (sortClause != NIL)
	{
		/* we need only check the first entry */
		SortBy *sby = (SortBy *) linitial(sortClause);

		siblings_by = sby->siblings;

		/* replace orderby aliasname */
		if (siblings_by)
			replace_orderby_aliasname(cur_pstate, stmt->targetList, sortClause);
	}
	if (whereClause != NULL)
	{
		ParseState	*tmp_pstate = make_parsestate(pstate);

		/*
		 * we use tmp_pstate,because transformWhereClause maybe add join clause for a(+) = b
		 * so we should use tmp_pstate which not handle add join cluase
		 * or we will get failed for "column reference xxx is ambiguous" in
		 * some walker function like split_join_where_clause_recurse or
		 * remove_tabreference_walker
		 */
		tmp_pstate->p_check_connectby = true;
		transformFromClause(tmp_pstate, copyObject(stmt->fromClause));
		transformWhereClause(tmp_pstate, copyObject(whereClause),
								EXPR_KIND_WHERE, "WHERE");
		free_parsestate(tmp_pstate);
	}

	/* handle start with contain level */
	stmt->whereClause = parsetree_tree_walker(stmt->whereClause,
				replace_series_walker, &replace_series_ctx);

	oldtarget = copyObject(stmt->targetList);

	oldtarget = (List *) parsetree_tree_walker((Node *) oldtarget,
						replace_priorcolname_walker, &replace_prior_ctx);
	sortClause = (List *) parsetree_tree_walker((Node *) sortClause,
						replace_priorcolname_walker, &replace_prior_ctx);
	limitOffset = parsetree_tree_walker(limitOffset,
						replace_priorcolname_walker, &replace_prior_ctx);
	limitCount = parsetree_tree_walker(limitCount,
						replace_priorcolname_walker, &replace_prior_ctx);
	lockingClause = (List *) parsetree_tree_walker((Node *) lockingClause,
						replace_priorcolname_walker, &replace_prior_ctx);
	groupClause = (List *) parsetree_tree_walker((Node *) groupClause,
						replace_priorcolname_walker, &replace_prior_ctx);
	havingClause = parsetree_tree_walker(havingClause,
						replace_priorcolname_walker, &replace_prior_ctx);
	/* Begin - bug#00008247 */
	distinctClause = (List *) parsetree_tree_walker((Node *) distinctClause,
						replace_priorcolname_walker, &replace_prior_ctx);
	windowClause = (List *) parsetree_tree_walker((Node *) windowClause,
						replace_priorcolname_walker, &replace_prior_ctx);
	/* End - bug#00008247 */

	/* check fromClause */
	if (list_length(stmt->fromClause) >= 2 ||
		(stmt->fromClause != NIL &&
		nodeTag(linitial(stmt->fromClause)) == T_JoinExpr))
	{
		RangeSubselect *rage;
		SelectStmt *newstmt;
		ListCell *lc;

		more_tables = true;
		/* only consider simpel table */
		has_no_special_cols = false;
		rage = makeNode(RangeSubselect);
		rage->alias = makeNode(Alias);
		rage->alias->aliasname = pstrdup(tb_alias);
		tbname = tb_alias;
		origin_tb_alias = false;

		newstmt = makeNode(SelectStmt);
		colres = makeNode(ResTarget);
		colall = makeNode(ColumnRef);
		colall->fields = list_make1(makeNode(A_Star));
		colres->val = (Node *)colall;

		newstmt->targetList = list_make1(colres);
		newstmt->fromClause = copyObject(stmt->fromClause);
		rage->subquery = (Node *) newstmt;

		/* more tables, we should recognize tables join clause */
		if (list_length(stmt->fromClause) >= 2 && whereClause != NULL)
		{
			Node *joinclause = copyObject(whereClause);
			Node *newwhereclause= copyObject(whereClause);
			List *table_indexs = NIL;
			bool has_prior = false;
			split_join_where_clause_context split_ctx;
			bool internel_outjoin = false;
			List *outjoin_indexs = NIL;

			/*
			 * whereclause has a outjoin, we should first get its outjoin'table informations
			 * because oracle handle a.id(+) = b.id, then all outjoin about table a columns
			 * will consider join_clause not where clause even that a outjoin which has
			 * reference only a column like a.xx(+) = 23
			 * so we should first get tables'informats if it has outjoin,
			 * so that, when we split join and where clause
			 * we can recognize it
			 */
			split_ctx.cur_pstate = cur_pstate;
			split_ctx.table_indexs = &table_indexs;
			split_ctx.replace_join = true;
			split_ctx.flags = PRE_AND_OPT;
			split_ctx.has_prior = &has_prior;
			split_ctx.has_outerjoin = false;
			split_ctx.je = NULL;
			split_ctx.internel_outerjoin = &internel_outjoin;
			split_ctx.outjoin_indexs = &outjoin_indexs;
			split_ctx.twice_replace_outjoin = false;

			split_join_where_clause_recurse(&newwhereclause,
											&split_ctx);
			if (outjoin_indexs)
			{
				list_free_deep(table_indexs);
				table_indexs = NIL;
				newwhereclause = copyObject(whereClause);
				has_prior = false;
				split_ctx.table_indexs = &table_indexs;
				split_ctx.replace_join = true;
				split_ctx.flags = PRE_AND_OPT;
				split_ctx.has_outerjoin = false;
				split_ctx.je = NULL;
				split_ctx.twice_replace_outjoin = true;

				split_join_where_clause_recurse(&newwhereclause,
								&split_ctx);
			}
			list_free_deep(table_indexs);
			table_indexs = NIL;
			has_prior = false;

			split_ctx.table_indexs = &table_indexs;
			split_ctx.replace_join = false;
			split_ctx.flags = PRE_AND_OPT;
			split_ctx.has_outerjoin = false;
			split_ctx.je = NULL;
			split_ctx.twice_replace_outjoin = true;

			split_join_where_clause_recurse(&joinclause, &split_ctx);

			list_free_deep(outjoin_indexs);
			list_free_deep(table_indexs);

			if (nodeTag(joinclause) != T_TypeCast ||
				nodeTag(((TypeCast *) joinclause)->arg) != T_A_Const)
				newstmt->whereClause = joinclause;
			if (nodeTag(newwhereclause) != T_TypeCast ||
				nodeTag(((TypeCast *) newwhereclause)->arg) != T_A_Const)
			{
				newwhereclause = parsetree_tree_walker(newwhereclause,
									remove_outjoin_walker, NULL);
				whereClause = newwhereclause;
			}
			else
				whereClause = NULL;
		}
		whereClause = parsetree_tree_walker(whereClause,
					replace_priorcolname_walker, &replace_prior_ctx);

		stmt->targetList = (List *) parsetree_tree_walker((Node *) stmt->targetList,
						remove_tabreference_walker, &remove_tab_ctx);

		oldtarget = expandTargetListAstar(cur_pstate, oldtarget);

		oldtarget = (List *) parsetree_tree_walker((Node *) oldtarget,
							remove_tabreference_walker, &remove_tab_ctx);

		whereClause = parsetree_tree_walker(whereClause,
					remove_tabreference_walker, &remove_tab_ctx);
		/* Begin - bug#00008247 */
		/*
		 * there, sortclause groupclause,havingclause maybe reference
		 * alias, so we need not to add tab reference
		 */
		remove_tab_ctx.only_replace = true;
		sortClause = (List *) parsetree_tree_walker((Node *) sortClause,
					remove_tabreference_walker, &remove_tab_ctx);
		groupClause = (List *) parsetree_tree_walker((Node *) groupClause,
			remove_tabreference_walker, &remove_tab_ctx);
		havingClause = parsetree_tree_walker(havingClause,
					remove_tabreference_walker, &remove_tab_ctx);
		remove_tab_ctx.only_replace = false;

		limitOffset = parsetree_tree_walker(limitOffset,
					remove_tabreference_walker, &remove_tab_ctx);
		limitCount = parsetree_tree_walker(limitCount,
					remove_tabreference_walker, &remove_tab_ctx);
		lockingClause = (List *) parsetree_tree_walker((Node *) lockingClause, 
					remove_tabreference_walker, &remove_tab_ctx);
		distinctClause = (List *) parsetree_tree_walker((Node *) distinctClause,
							remove_tabreference_walker, &remove_tab_ctx);
		windowClause = (List *) parsetree_tree_walker((Node *) windowClause,
							remove_tabreference_walker, &remove_tab_ctx);
		/* End - bug#00008247 */

		remove_tab_ctx.tbname = internel_priorname;
		stmt->whereClause = parsetree_tree_walker(stmt->whereClause,
						remove_tabreference_walker, &remove_tab_ctx);
		lq->connect_by = parsetree_tree_walker(lq->connect_by,
					remove_tabreference_walker, &remove_tab_ctx);
		remove_tab_ctx.tbname = allfinalname;

		table_column_list = append_table_columnlist(table_column_list, prior_colname_list);

		foreach (lc, table_column_list)
		{
			ColumnRef *ref = (ColumnRef *) lfirst(lc);
			ResTarget *res;
			char *nameStr;
			StringInfoData strinfo;

			nameStr = NameListToString(ref->fields);
			initStringInfo(&strinfo);
			appendStringInfo(&strinfo, "%s%s", prefix_table, nameStr);
			pfree(nameStr);
			res = makeNode(ResTarget);
			res->name = strinfo.data;
			res->val = (Node *) ref;

			newstmt->targetList = lappend(newstmt->targetList, res);
			(*ignoreStr)->lq_table_col = lappend((*ignoreStr)->lq_table_col, makeString(strinfo.data));
		}

		list_free(table_column_list);
		remove_tab_ctx.columnlist = NULL;
		unionnode = (Node *) rage;
	}
	else if (1 == list_length(stmt->fromClause))
	{
		unionnode = linitial(stmt->fromClause);

		whereClause = parsetree_tree_walker(whereClause,
			replace_priorcolname_walker, &replace_prior_ctx);
		if (unionnode != NULL && T_RangeVar == nodeTag(unionnode))
		{
			rv = (RangeVar *)unionnode;
			if (rv->alias)
			{
				tbname = pstrdup(rv->alias->aliasname);
			}
			else
			{
				tbname = pstrdup(rv->relname);
			}
		}
		else if (T_RangeSubselect == nodeTag(unionnode))
		{
			RangeSubselect *rsub = (RangeSubselect *) unionnode;

			if (rsub->alias)
				tbname = pstrdup(rsub->alias->aliasname);
			else
			{
				rsub->alias = makeNode(Alias);
				rsub->alias->aliasname = pstrdup(tb_alias);
				tbname = tb_alias;
				origin_tb_alias = false;
			}
			/* only consider simpel table */
			has_no_special_cols = false;
		}
		else
		{
			ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				errmsg("start with ...connect by ... only support table/view/subselect")));
		}
		stmt->whereClause = parsetree_tree_walker(stmt->whereClause,
			add_tabreference_walker, (void *) &tab_ctx);
	}
	else
	{
		ereport(ERROR,
			(errcode(ERRCODE_SYNTAX_ERROR),
			errmsg("start with ...connect by ... should has a table")));
	}

	/* wether it is a sibling by */
	if (siblings_by)
	{
		tab_ctx.remove_schema = false;
		sortClause = (List *) parsetree_tree_walker((Node *) sortClause,
			add_tabreference_walker, (void *) &tab_ctx);
		tab_ctx.remove_schema = true;
	}

	/* handle other clause include pseudo column */
	whereClause = parsetree_tree_walker(whereClause,
			replace_pseduocol_walker, &pseudocol_ctx);
	sortClause = (List *) parsetree_tree_walker((Node *) sortClause,
			replace_pseduocol_walker, &pseudocol_ctx);
	limitOffset = parsetree_tree_walker(limitOffset,
			replace_pseduocol_walker, &pseudocol_ctx);
	limitCount = parsetree_tree_walker(limitCount,
			replace_pseduocol_walker, &pseudocol_ctx);
	lockingClause = (List *) parsetree_tree_walker((Node *) lockingClause,
			replace_pseduocol_walker, &pseudocol_ctx);
	groupClause = (List *) parsetree_tree_walker((Node *) groupClause,
			replace_pseduocol_walker, &pseudocol_ctx);
	havingClause = parsetree_tree_walker(havingClause,
		replace_pseduocol_walker, &pseudocol_ctx);
	/* Begin - bug#00008247 */
	distinctClause = (List *) parsetree_tree_walker((Node *) distinctClause,
						replace_pseduocol_walker, &pseudocol_ctx);
	windowClause = (List *) parsetree_tree_walker((Node *) windowClause,
						replace_pseduocol_walker, &pseudocol_ctx);
	/* End - bug#00008247 */

	allfinaltarget = (List *) copyObject(oldtarget);

	/*
	 * check target table name and pseudo column
	 * two tables or more than two tables, we has replaced it
	 * to allfinalname
	 */
	if (origin_tb_alias)
	{
		check_target_tbname_context check_tbname_context;

		check_tbname_context.cur_pstate = cur_pstate;
		check_tbname_context.tbname = tbname;
		/* Begin - bug#00008247 */
		check_tbname_context.sublevel = 0;
		check_tbname_context.child_pstate = NULL;
		/* End - bug#00008247 */

		oldtarget = (List *) parsetree_tree_walker((Node *) oldtarget,
				check_target_tname_walker, (void *) &check_tbname_context);
	}

	/* make with level__query1 as (select * from unioncode) */
	colres = makeNode(ResTarget);
	colall = makeNode(ColumnRef);
	colall->fields = list_make1(makeNode(A_Star));

	colres->val = (Node *)colall;

	internel_select = (SelectStmt *) makeNode(SelectStmt);
	internel_select->targetList = list_make1(colres);
	internel_select->fromClause = list_make1(unionnode);

	internel_cte->ctename = internel_priorname;
	internel_cte->ctequery = (Node *) internel_select;
	internel_with->recursive = false;
	internel_with->location = -1;
	internel_with->ctes = list_make1(internel_cte);
	stmt->fromClause = list_make1(makeRangeVar(NULL, internel_priorname, -1));

	oldtarget = (List *) parsetree_tree_walker((Node *) oldtarget,
					replace_pseduocol_walker, &pseudocol_ctx);
	haslevel = ((psecol_flags & CONNECT_BY_CONTAIN_LEVEL) != 0);
	hasisleaf = ((psecol_flags & CONNECT_BY_CONTAIN_LEAF) != 0);
	hascycle = ((psecol_flags & CONNECT_BY_CONTAIN_CYCLE) != 0);

	/* handle connect by */
	origin_connect_by = copyObject(lq->connect_by);
	connect_by_rownum = copyObject(origin_connect_by);


	replace_series_ctx.prior_columlist = &connect_by_priorlist;
	replace_series_ctx.prior_name = priorname;
	replace_series_ctx.tbname = internel_priorname;
	replace_series_ctx.found_prior = &connect_by_prior;
	replace_series_ctx.recurse_flag = &connect_by_flag;

	lq->connect_by = parsetree_tree_walker(lq->connect_by,
		replace_series_walker, &replace_series_ctx);

	/*
	 * there, no prior operator
	 * and has no parent level column
	 * reference
	 */
	if (prior_colname_list != NIL ||
		syspathlist != NIL ||
		rootlist != NIL ||
		hascycle ||
		hasisleaf ||
		connect_by_prior||
		connect_by_flag & CONNECT_BY_CONTAIN_LEVEL ||
		!connect_by_rownum_use_func ||
		sortClause ||
		!(connect_by_flag & CONNECT_BY_CONTAIN_ROWNUM) ||
		SPI_get_connected() >= 0 || /* not plsql */
		pstate->p_paramref_hook != NULL) /* has parameter */
		has_no_special_cols = false;

	if (connect_by_flag & CONNECT_BY_CONTAIN_ROWNUM)
		connect_by_rownum = parsetree_tree_walker(connect_by_rownum,
			replace_priorcolname_walker, &replace_prior_ctx);

	/* get origin with columns list */
	if (has_no_special_cols)
	{
		ParseState	*tmp_pstate = make_parsestate(pstate);
		int rtindex;
		RangeTblEntry *tbl;
		Node *conect_by_node;
		List *colrefs;
		ListCell *lc;

		transformFromClause(tmp_pstate, copyObject(internel_select->fromClause)); /* BUG#Z00005447 */
		rtindex = list_length(tmp_pstate->p_rtable);
		tbl = rt_fetch(rtindex, tmp_pstate->p_rtable);
		Assert(tbl->eref != NULL && tbl->eref->colnames != NIL);
		expandRTE(tbl, rtindex, 0, -1, false,
				  &base_colnames, NULL);
		conect_by_node = transformWhereClause(tmp_pstate, origin_connect_by,
								EXPR_KIND_WHERE, "WHERE");
		colrefs = pull_var_clause(conect_by_node,
										   PVC_RECURSE_AGGREGATES |
										   PVC_INCLUDE_PLACEHOLDERS);
		/* check not reference out vars */
		foreach (lc, colrefs)
		{
			Var *var = (Var *) lfirst(lc);

			if (var->varlevelsup != 0)
			{
				has_no_special_cols = false;
				break;
			}
		}
		list_free(colrefs);
		/*
		 * check start with
		 */
		if (has_no_special_cols)
		{
			conect_by_node = transformWhereClause(tmp_pstate, origin_start_with,
									EXPR_KIND_WHERE, "WHERE");
			colrefs = pull_var_clause(conect_by_node,
											   PVC_RECURSE_AGGREGATES |
											   PVC_INCLUDE_PLACEHOLDERS);
			foreach (lc, colrefs)
			{
				Var *var = (Var *) lfirst(lc);

				if (var->varlevelsup != 0)
				{
					has_no_special_cols = false;
					break;
				}
			}
			list_free(colrefs);
		}
		if (!has_no_special_cols)
		{
			list_free(base_colnames);
			base_colnames = NIL;
		}
		free_parsestate(tmp_pstate);
	}
	if (has_no_special_cols)
	{
		Query *query;
		char *initsql;
		char *connect_bysql;
		SelectStmt *temp_select;
		ParseState	*tmp_pstate = make_parsestate(pstate);
		RawStmt		*top_stmt = makeNode(RawStmt);

		temp_select = (SelectStmt *) copyObject(internel_select);
		temp_select->whereClause = origin_start_with;
		top_stmt->stmt = (Node *) temp_select;
		top_stmt->stmt_location = -1;
		top_stmt->stmt_len = 0;

		/*add 1 as level into target list */
		level1 = makeNode(ResTarget);
		level1->name = levelcol;
		level1->val = makeIntConst(1);
		temp_select->targetList = lappend(temp_select->targetList,
									level1);
		query = transformTopLevelStmt(tmp_pstate, top_stmt);
		initsql = ivy_get_query_def(query);
		/* construct connect_bysql */
		{
			int numParams = 2;
			Oid		   *paramTypes = NULL;
			ParamRef	*param = makeNode(ParamRef);
			char 	   *paramModes = NULL;

			paramTypes = palloc(sizeof(Oid) * numParams);
			paramTypes[0] = INT4OID;
			paramTypes[1] = NUMBEROID;

			temp_select = (SelectStmt *) copyObject(internel_select);
			/* add $2 as level into target list */
			level1 = makeNode(ResTarget);
			level1->name = levelcol;
			param->number = 2;
			level1->val = (Node *) param;
			temp_select->targetList = lappend(temp_select->targetList,
										level1);
			list_free(connect_by_priorlist);
			connect_by_priorlist = NIL;
			lq->connect_by = copyObject(origin_connect_by);
			lq->connect_by = parsetree_tree_walker(lq->connect_by,
				replace_rownum_to_param_walker, NULL);
			temp_select->whereClause = lq->connect_by;
			top_stmt->stmt = (Node *) temp_select;
			query = parse_analyze_varparams(top_stmt, source_sql,
									&paramTypes, &numParams,
									&paramModes);
			connect_bysql = ivy_get_query_def(query);
		}
		pfree(top_stmt);

		/* select * from _connect_by_rownum_query(initsql,connect_bysql) */
		{
			FuncCall *connect_byfunc;
			ListCell *lc;
			Alias *a = makeNode(Alias);

			/* make _connect_by_rownum_query func */
			connect_byfunc = makeFuncCall(list_make1(makeString(pstrdup(connect_by_rownum_func_str))),
					list_make2(makeStringConst(initsql), makeStringConst(connect_bysql)),
					COERCE_SQL_SYNTAX,
					-1);
			a->aliasname = priorname;
			rangefunc = makeNode(RangeFunction);
			rangefunc->alias = a;
			rangefunc->functions = list_make1(list_make2(connect_byfunc, NIL));
			foreach(lc, base_colnames)
			{
				Value *colname = (Value *) lfirst(lc);
				Oid coloid;
				TypeName *typname;
				ColumnRef *ref = (ColumnRef *) makeColumnRef(strVal(colname), NIL);
				ColumnDef *coldef = makeNode(ColumnDef);
				int32 colmod;
				Oid colcollation;

				coloid = getColumnTypeidFromColumnref(cur_pstate, ref, &colmod,
													&colcollation);
				typname =  makeTypeNameFromOid(coloid, colmod);
				coldef->colname = strVal(colname);
				coldef->typeName = typname;
				coldef->is_local = true;
				coldef->location = -1;
				rangefunc->coldeflist = lappend(rangefunc->coldeflist, coldef);
			}
			/* append level int as */
			{
				ColumnDef *coldef = makeNode(ColumnDef);

				coldef->colname = levelcol;
				coldef->typeName = makeTypeNameFromOid(INT4OID, -1);;
				coldef->is_local = true;
				coldef->location = -1;
				rangefunc->coldeflist = lappend(rangefunc->coldeflist, coldef);
			}
		}
		free_parsestate(tmp_pstate);
	}

	/* connect_by_iscycle require nocycle property */
	if (hascycle && !lq->is_nocycle)
		elog(ERROR, "NOCYCLE keyword is required with CONNECT_BY_ISCYCLE pseudocolumn");

	/* make left select *  ....*/
	if (!has_no_special_cols)
	{
		{
			targetnode = makeNode(ResTarget);
			all = makeNode(ColumnRef);
			all->fields = list_make1(makeNode(A_Star));

			targetnode->val = (Node *)all;

			stmt->targetList = list_make1(targetnode);
		}
		/* make prior colname */
		if (prior_colname_list != NULL)
		{
			ListCell *lc;
			foreach (lc, prior_colname_list)
			{
				ColumnRef *ref = (ColumnRef *) lfirst(lc);
				char *refname;
				StringInfoData infodata;
				ResTarget *res;
				A_Const *nul;
				TypeCast *typcast;
				Oid		coloid;
				int32 colmod;
				Oid		colcollation = InvalidOid;

				nul = makeNode(A_Const);
				nul->val.type = T_Null;
				nul->location = -1;

				typcast = makeNode(TypeCast);
				refname = NameListToString(ref->fields);

				coloid = getColumnTypeidFromColumnref(cur_pstate, ref, &colmod, &colcollation);
				typcast->typeName = makeTypeNameFromOid(coloid, colmod);
				typcast->arg = (Node *) nul;

				initStringInfo(&infodata);
				appendStringInfo(&infodata, "%s%s", prior_colname_prefix, refname);
				pfree(refname);

				if (OidIsValid(colcollation))
				{
					CollateClause *n = makeNode(CollateClause);

					n->arg = (Node *) typcast;
					n->collname = get_collation_name_forlist(colcollation);
					n->location = -1;

					typcast = (TypeCast *) n;
				}

				res = makeNode(ResTarget);
				res->name = infodata.data;
				res->val = (Node *) typcast;
				stmt->targetList = lappend(stmt->targetList, res);
				(*ignoreStr)->lq_prior = lappend((*ignoreStr)->lq_prior, makeString(infodata.data));
			}
		}

		/* make left sys_connect_by_path function*/
		if (syspathlist != NIL)
		{
			ListCell *lc;
			Node *castnode;
			SysConnectPath *syspath;

			foreach (lc, syspathlist)
			{
				syspath = (SysConnectPath *) lfirst(lc);

				syspath->colnode= parsetree_tree_walker(syspath->colnode,
					add_tabreference_walker, &tab_ctx);
				castnode = (Node *)makeA_Expr(AEXPR_OP, list_make1(makeString(pstrdup("||"))),
										makeStringConst(syspath->joinOp),
										syspath->colnode,
										-1);

				targetnode = makeNode(ResTarget);
				targetnode->name = syspath->alias;
				targetnode->val = castnode;
				stmt->targetList = lappend(stmt->targetList, targetnode);
				(*ignoreStr)->lq_path = lappend((*ignoreStr)->lq_path, makeString(targetnode->name));
			}
		}

		/* make left connect_by_root function*/
		if (rootlist != NIL)
		{
			ListCell *lc;
			ConnectByRoot *root;

			foreach (lc, rootlist)
			{
				root = (ConnectByRoot *) lfirst(lc);

				targetnode = makeNode(ResTarget);
				targetnode->name = root->alias;
				root->colnode = parsetree_tree_walker(root->colnode,
					add_tabreference_walker, &tab_ctx);
				targetnode->val = root->colnode;
				stmt->targetList = lappend(stmt->targetList, targetnode);
				(*ignoreStr)->lq_root = lappend((*ignoreStr)->lq_root, makeString(root->alias));
			}
		}

		/* Begin - BUG#Z00004575 */
		/*
		 * there root row connect_by_iscycle maybe
		 * true
		 */
		/* make left connect_by_iscycle pseudocolumn */
		if (connect_by_prior)
		{
			if (hascycle)
			{
				/*
				 * construct connect_by_iscycle
				 * case WHEN 1 <= (select count(*) from level__query1  where connectby and
				 * row(_tbname_1.prior colist) = row(level__query1.prior colist))
				 * then 1 else 0 END as connect_by_iscycle
				 */
				SelectStmt *st;
				FuncCall *fcall;
				CaseExpr *caseiscycle;
				List *caseiscycle_args = NIL;
				ResTarget *cyclecol = makeNode(ResTarget);
				Node *connectcycle;
				SubLink *sub;
				ResTarget *res;
				RangeVar *rvar;
				Alias *alias;
				char *tb_alias1 = "_tbname_1";
				int cycle_flags = 0;
				bool cycle_found_prior = false;
				RowExpr *r;
				RowExpr *r1;

				st = makeNode(SelectStmt);
				rvar = makeRangeVar(NULL, internel_priorname, -1);

				alias = (Alias *) makeNode(Alias);
				alias->aliasname = pstrdup(tb_alias1);

				rvar->alias = alias;

				st->fromClause = list_make1(rvar);

				fcall = makeNode(FuncCall);
				fcall->funcname = list_make1(makeString("count"));
				fcall->agg_star = true;

				res = makeNode(ResTarget);
				res->location = -1;
				res->val = (Node *) fcall;

				st->targetList = list_make1(res);

				/* construct where clause */
				connectcycle = copyObject(origin_connect_by);

				replace_series_ctx.prior_columlist = NULL;
				replace_series_ctx.prior_name = internel_priorname;
				replace_series_ctx.tbname = tb_alias1;
				replace_series_ctx.found_prior = &cycle_found_prior;
				replace_series_ctx.recurse_flag = &cycle_flags;

				connectcycle = parsetree_tree_walker(connectcycle,
						replace_series_walker, &replace_series_ctx);


				r = makeNode(RowExpr);

				r->row_typeid = InvalidOid;
				r->colnames = NIL;
				r->row_format = COERCE_EXPLICIT_CALL;
				r->location = -1;
				if (connect_by_priorlist != NIL)
				{
					tab_ctx.tbname = tb_alias1;

					r->args = copyObject(connect_by_priorlist);

					r->args = (List *) parsetree_tree_walker((Node *) r->args,
								add_tabreference_walker, (void *) &tab_ctx);
					tab_ctx.tbname = internel_priorname;
				}
				else
				{
					Node *rnode;

					rnode = makeColumnRef(tb_alias1, list_make1(makeNode(A_Star)));
					r->args = list_make1(rnode);
				}

				r1 = makeNode(RowExpr);

				r1->row_typeid = InvalidOid;
				r1->colnames = NIL;
				r1->row_format = COERCE_EXPLICIT_CALL;
				r1->location = -1;
				if (connect_by_priorlist != NIL)
				{

					r1->args = copyObject(connect_by_priorlist);

					r1->args = (List *) parsetree_tree_walker((Node *) r1->args,
								add_tabreference_walker, (void *) &tab_ctx);
				}
				else
				{
					Node *rnode;

					rnode = makeColumnRef(internel_priorname, list_make1(makeNode(A_Star)));
					r1->args = list_make1(rnode);
				}

				st->whereClause = (Node *) makeSimpleA_Expr(AEXPR_OP, "=", (Node *) r,
													(Node *) r1, -1);
				st->whereClause = makeAndExpr(connectcycle, st->whereClause, -1);

				/* make sublink */
				sub = makeNode(SubLink);
				sub->subLinkType = EXPR_SUBLINK;
				sub->subLinkId = 0;
				sub->testexpr = NULL;
				sub->operName = NIL;
				sub->subselect = (Node *) st;
				sub->location = -1;

				/* make case */
				caseiscycle = makeNode(CaseExpr);
				casewheniscycle = makeNode(CaseWhen);
				casewheniscycle->expr = (Expr *)makeSimpleA_Expr(AEXPR_OP, "<=", makeIntConst(1),
												(Node *)sub, -1);
				casewheniscycle->result = (Expr *)makeIntConst(1);
				caseiscycle_args = lappend(caseiscycle_args, casewheniscycle);
				caseiscycle->args = caseiscycle_args;
				caseiscycle->defresult = (Expr *)makeIntConst(0);

				cyclecol->name = iscyclename;
				cyclecol->val = (Node *) caseiscycle;
				stmt->targetList = lappend(stmt->targetList, cyclecol);
				(*ignoreStr)->lq_connect_by_iscycle = iscyclename;
			}
		}
		else if (hascycle)
		{
			/* connect by series connect_by_iscycle is awalys 0 */
			ResTarget *cyclecol = makeNode(ResTarget);

			cyclecol->name = iscyclename;
			cyclecol->val = makeIntConst(0);
			stmt->targetList = lappend(stmt->targetList, cyclecol);
			(*ignoreStr)->lq_connect_by_iscycle = iscyclename;
		}
		/* End - BUG#Z00004575 */

		/*
		 * make left level col
		 * connect by level/rownum depend on level
		 */
		if (haslevel || hasisleaf || connect_by_flag != 0)
		{
			level1 = makeNode(ResTarget);
			level1->name = levelcol;
			level1->val = makeIntConst(1);
			stmt->targetList = lappend(stmt->targetList, level1);
		}

		/* make left array[] node*/
		{
			/* make array[row_number() over()] begin */
			WindowDef *n = makeNode(WindowDef);
			n->frameOptions = FRAMEOPTION_DEFAULTS;

			if (siblings_by)
				n->orderClause = sortClause;

			rowfun = makeNode(FuncCall);
			rowfun->funcname = list_make1(makeString(pstrdup("row_number")));
			rowfun->over = n;

			array1 = makeNode(A_ArrayExpr);
			array1->elements = list_make1(rowfun);
			res = makeNode(ResTarget);
			res->name = deeporderbycol;
			res->val = (Node*)array1;
			stmt->targetList = lappend(stmt->targetList, res);
			/* make array[row_number() over()] end */
			/* series doesn't need cte_0_array__path */
			if (connect_by_prior)
			{
				/* make columnref cte_0_array__path begin */
				RowExpr *r = makeNode(RowExpr);

				r->row_typeid = InvalidOid;
				r->colnames = NIL;
				r->row_format = COERCE_EXPLICIT_CALL;
				r->location = -1;
				/* if no prior column, we use the whole row */
				if (connect_by_priorlist == NIL)
				{
					nodepath1 = makeColumnRef(internel_priorname, list_make1(makeNode(A_Star)));
					r->args = list_make1(nodepath1);
				}
				else
				{
					connect_by_priorlist = (List *) parsetree_tree_walker((Node *) connect_by_priorlist,
						add_tabreference_walker, (void *) &tab_ctx);
					r->args = connect_by_priorlist;
				}

				nodecastpath1 = makeNode(TypeCast);
				nodecastpath1->arg = (Node *) r;
				nodecastpath1->typeName = makeTypeName("text");
				nodecastpath1->location = -1;

				arraypath = makeNode(A_ArrayExpr);
				arraypath->elements = list_make1(nodecastpath1);

				arrayres = makeNode(ResTarget);
				arrayres->name = ctepath;
				arrayres->val = (Node*)arraypath;
				stmt->targetList = lappend(stmt->targetList, arrayres);
				/* make columnref cte_0_array__path end */
			}

			lStmt = stmt;
			rStmt = makeNode(SelectStmt);

			/* make right select *  ....*/
			{
				node = makeColumnRef(internel_priorname, list_make1(makeNode(A_Star)));
				targetnode = makeNode(ResTarget);
				targetnode->val = node;
				rStmt->targetList = list_make1(targetnode);
			}

			/* append prior column list */
			if (prior_colname_list != NIL)
			{
				ListCell *lc;

				tab_ctx.tbname = priorname;

				foreach (lc, prior_colname_list)
				{
					ColumnRef *ref = (ColumnRef *) copyObject(lfirst(lc));
					ResTarget *res = makeNode(ResTarget);
					char *namestr;
					StringInfoData infodata;

					namestr = NameListToString(ref->fields);
					initStringInfo(&infodata);

					appendStringInfo(&infodata, "%s%s", prior_colname_prefix, namestr);
					res->name = infodata.data;
					if (!more_tables)
						ref = (ColumnRef *) parsetree_tree_walker((Node *) ref,
								add_tabreference_walker, (void *) &tab_ctx);
					else
					{
						remove_tab_ctx.columnlist = NULL;
						remove_tab_ctx.tbname = priorname;

						ref = (ColumnRef *) parsetree_tree_walker((Node *) ref,
								remove_tabreference_walker, &remove_tab_ctx);
					}
					res->val = (Node *) ref;
					pfree(namestr);
					rStmt->targetList = lappend(rStmt->targetList, res);
				}
				tab_ctx.tbname = internel_priorname;
			}

			/* make right sys_connect_by_path function*/
			if (syspathlist != NIL)
			{
				Node *castnode;
				Node *ref;
				ListCell *lc;
				SysConnectPath *syspath;

				foreach (lc, syspathlist)
				{
					syspath = (SysConnectPath *) lfirst(lc);

					ref = (Node *) copyObject(syspath->colnode);
					ref = parsetree_tree_walker(ref,
						add_tabreference_walker, (void *) &tab_ctx);

					castnode = (Node *)makeA_Expr(AEXPR_OP, list_make1(makeString(pstrdup("||"))),
										makeColumnRef(priorname,
										list_make1(makeString(syspath->alias))),
										makeStringConst(syspath->joinOp),
										-1);
					castnode = (Node *)makeA_Expr(AEXPR_OP, list_make1(makeString(pstrdup("||"))),
											castnode,
											ref,
											-1);
					targetnode = makeNode(ResTarget);
					targetnode->name = syspath->alias;
					targetnode->val = castnode;
					rStmt->targetList = lappend(rStmt->targetList, targetnode);
				}
			}

			/* make right connect_by_root function*/
			if (rootlist != NIL)
			{
				Node *castnode;
				ListCell *lc;
				ConnectByRoot *root;

				foreach (lc, rootlist)
				{
					root = (ConnectByRoot *) lfirst(lc);

					castnode = (Node*)makeColumnRef(priorname, list_make1(makeString(root->alias)));

					targetnode = makeNode(ResTarget);
					targetnode->name = root->alias;
					targetnode->val = castnode;
					rStmt->targetList = lappend(rStmt->targetList, targetnode);
				}
			}

			/* make right connect_by_iscycle pseudocolumn */
			if (connect_by_prior)
			{
				/* make level__query.cte_0_array__path and (ROW(tbname.*)::text)) */
				RowExpr *r = makeNode(RowExpr);

				r->row_typeid = InvalidOid;
				r->colnames = NIL;
				r->row_format = COERCE_EXPLICIT_CALL;
				r->location = -1;

				rwherenode = (Node*)makeColumnRef(priorname, list_make1(makeString(ctepath)));
				/* if no prior column, we use the whole row */
				if (connect_by_priorlist == NIL)
				{
					rnode = makeColumnRef(internel_priorname, list_make1(makeNode(A_Star)));
					r->args = list_make1(rnode);
				}
				else
					r->args = copyObject(connect_by_priorlist);
				rnodecast = makeNode(TypeCast);
				rnodecast->arg = (Node *) r;
				rnodecast->typeName = makeTypeName("text");
				rnodecast->location = 0;
				nocyclenode = (Node *)makeBoolAConst(lq->is_nocycle);//if exist NOCYCLE
				rwherefunc = makeNode(FuncCall);
				rwherefunc->funcname = list_make1(makeString(pstrdup("cycle_check")));
				rwherefunc->args = list_make3(rwherenode, rnodecast, nocyclenode);

				if (hascycle)
				{
					/*
					 * construct connect_by_iscycle
					 * case WHEN 1 <= (select count(*) from tbname _tbname_1 where connectby
					 * and ROW(_tbname_.employee_id)::text = ANY(level__query.cte_0_array__path))
					 * then 1 else 0 END as connect_by_iscycle
					 */
					SelectStmt *st;
					FuncCall *fcall;
					CaseExpr *caseiscycle;
					List *caseiscycle_args = NIL;
					ResTarget *cyclecol = makeNode(ResTarget);
					Node *connectcycle;
					int cycle_flags = 0;
					bool cycle_found_prior = false;
					RowExpr *r;
					TypeCast *cyclenodecast;
					SubLink *sub;
					ResTarget *res;
					Node *anynode;
					RangeVar *rvar;
					Alias *alias;
					char *tb_alias1 = "_tbname_1";

					st = makeNode(SelectStmt);
					rvar = makeRangeVar(NULL, internel_priorname, -1);

					alias = (Alias *) makeNode(Alias);
					alias->aliasname = pstrdup(tb_alias1);

					rvar->alias = alias;

					st->fromClause = list_make1(rvar);

					fcall = makeNode(FuncCall);
					fcall->funcname = list_make1(makeString("count"));
					fcall->agg_star = true;

					res = makeNode(ResTarget);
					res->location = -1;
					res->val = (Node *) fcall;

					st->targetList = list_make1(res);

					/* construct where clause */
					connectcycle = copyObject(origin_connect_by);

					replace_series_ctx.prior_columlist = NULL;
					replace_series_ctx.prior_name = internel_priorname;
					replace_series_ctx.tbname = tb_alias1;
					replace_series_ctx.found_prior = &cycle_found_prior;
					replace_series_ctx.recurse_flag = &cycle_flags;

					connectcycle = parsetree_tree_walker(connectcycle,
							replace_series_walker, &replace_series_ctx);

					r = makeNode(RowExpr);

					r->row_typeid = InvalidOid;
					r->colnames = NIL;
					r->row_format = COERCE_EXPLICIT_CALL;
					r->location = -1;
					if (connect_by_priorlist != NIL)
					{
						tab_ctx.tbname = tb_alias1;

						r->args = copyObject(connect_by_priorlist);

						r->args = (List *) parsetree_tree_walker((Node *) r->args,
									add_tabreference_walker, (void *) &tab_ctx);
						tab_ctx.tbname = internel_priorname;
					}
					else
					{
						Node *rnode;

						rnode = makeColumnRef(internel_priorname, list_make1(makeNode(A_Star)));
						r->args = list_make1(rnode);
					}
					cyclenodecast = makeNode(TypeCast);
					cyclenodecast->arg = (Node *) r;
					cyclenodecast->typeName = makeTypeName("text");
					cyclenodecast->location = -1;

					anynode = makeColumnRef(priorname, list_make1(makeString(ctepath)));

					st->whereClause = (Node *) makeSimpleA_Expr(AEXPR_OP_ANY, "=", (Node *) cyclenodecast,
														anynode, -1);
					st->whereClause = makeAndExpr(connectcycle, st->whereClause, -1);

					/* make sublink */
					sub = makeNode(SubLink);
					sub->subLinkType = EXPR_SUBLINK;
					sub->subLinkId = 0;
					sub->testexpr = NULL;
					sub->operName = NIL;
					sub->subselect = (Node *) st;
					sub->location = -1;

					/* make case */
					caseiscycle = makeNode(CaseExpr);
					casewheniscycle = makeNode(CaseWhen);
					casewheniscycle->expr = (Expr *)makeSimpleA_Expr(AEXPR_OP, "<=", makeIntConst(1),
													(Node *)sub, -1);
					casewheniscycle->result = (Expr *)makeIntConst(1);
					caseiscycle_args = lappend(caseiscycle_args, casewheniscycle);
					caseiscycle->args = caseiscycle_args;
					caseiscycle->defresult = (Expr *)makeIntConst(0);

					cyclecol->name = iscyclename;
					cyclecol->val = (Node *) caseiscycle;
					rStmt->targetList = lappend(rStmt->targetList, cyclecol);
				}
			}
			else if (hascycle)
			{
				/* connect by series connect_by_iscycle is awalys 0 */
				ResTarget *cyclecol = makeNode(ResTarget);

				cyclecol->name = iscyclename;
				cyclecol->val = makeIntConst(0);
				rStmt->targetList = lappend(rStmt->targetList, cyclecol);
			}

			/* make right level col*/
			if (haslevel || hasisleaf || connect_by_flag != 0)
			{
				level1 = makeNode(ResTarget);
				level1->name = levelcol;
				level1->val = (Node *)makeSimpleA_Expr(AEXPR_OP, "+",
											makeColumnRef(priorname, list_make1(makeString(levelcol))),
											makeIntConst(1),
											-1);
				rStmt->targetList = lappend(rStmt->targetList, level1);
			}

			/* make right array_append node*/
			{
				/* make array_append(level__query.cte_0_by_order_col,row_number() over()) begin */
				ColumnRef *col;
				ColumnRef *colpath;

				col = (ColumnRef *)makeNode(ColumnRef);
				col->fields = list_make2(makeString(priorname), makeString(deeporderbycol));
				rowfun = copyObject(rowfun);
				arrayfun = makeNode(FuncCall);
				arrayfun->funcname = list_make1(makeString(pstrdup("array_append")));
				arrayfun->args = list_make2(col, rowfun);

				res = makeNode(ResTarget);
				res->name = deeporderbycol;
				res->val = (Node*)arrayfun;
				rStmt->targetList = lappend(rStmt->targetList, res);
				/* make array_append(level__query.cte_0_by_order_col,row_number() over()) end */

				if (connect_by_prior)
				{
					/* make new columnref cte_0_array__path begin */
					RowExpr *r = makeNode(RowExpr);

					r->row_typeid = InvalidOid;
					r->colnames = NIL;
					r->row_format = COERCE_EXPLICIT_CALL;
					r->location = -1;

					/* if no prior column, we use the whole row */
					if (connect_by_priorlist == NIL)
					{
						nodepath2 = makeColumnRef(internel_priorname, list_make1(makeNode(A_Star)));
						r->args = list_make1(nodepath2);
					}
					else
						r->args = copyObject(connect_by_priorlist);

					nodecastpath2 = makeNode(TypeCast);
					nodecastpath2->arg = (Node *) r;
					nodecastpath2->typeName = makeTypeName("text");
					nodecastpath2->location = -1;

					colpath = (ColumnRef *)makeNode(ColumnRef);
					colpath->fields = list_make2(makeString(priorname), makeString(ctepath));
					arraypathfun = makeNode(FuncCall);
					arraypathfun->funcname = list_make1(makeString(pstrdup("array_append")));
					arraypathfun->args = list_make2(colpath, nodecastpath2);

					arrayres = makeNode(ResTarget);
					arrayres->name = ctepath;
					arrayres->val = (Node*)arraypathfun;
					rStmt->targetList = lappend(rStmt->targetList, arrayres);
					/* make new columnref cte_0_array__path end */
				}
			}
			rStmt->fromClause = list_make2(makeRangeVar(NULL, internel_priorname, -1),
				makeRangeVar(NULL, priorname, -1));
			if (connect_by_prior)
				rsimplenode = makeAndExpr((Node*)rwherefunc, lq->connect_by, -1);
			else
				rsimplenode = lq->connect_by;

			rStmt->whereClause = rsimplenode;

			unionstmt = (SelectStmt*)makeSetOp(SETOP_UNION, true, (Node*)lStmt, (Node*)rStmt);	/**make union all**/
		}
		/*
		 * make with recursive level__query as (
		 * with level__query1 as (select columnlist from unioncode)
		 */
		unionstmt->withClause = internel_with;

		/* CTE: WITH RECURSIVE level__query  BEGIN */
		cte->ctename = priorname;
		cte->ctequery = (Node*)unionstmt;				/*make cte*/

		tailwithclause->ctes = list_make1(cte);			/*make tailwithclause*/
		tailwithclause->recursive = true;

		tailstmt = makeNode(SelectStmt);				/*make tail stmt*/
		tailstmt->withClause = tailwithclause;

		/* make select *  ....*/
		{
			node = makeColumnRef(priorname, list_make1(makeNode(A_Star)));
			targetnode = makeNode(ResTarget);
			targetnode->val = node;
			tailstmt->targetList = list_make1(targetnode);
		}

		/* from level__query */
		tailfromclause = list_make1(makeRangeVar(NULL, priorname, -1));
		tailstmt->fromClause = tailfromclause;

		/* order by cte_0_by_order_col */
		sortnode = makeNode(SortBy);
		sortnode->node = makeColumnRef(deeporderbycol, NIL);
		sortnode->sortby_dir = SORTBY_DEFAULT;
		sortnode->sortby_nulls = SORTBY_NULLS_DEFAULT;
		tailstmt->sortClause = list_make1(sortnode);
	}
	else
	{
		ColumnRef *colref;

		Assert(rangefunc != NULL);

		tailstmt = makeNode(SelectStmt);
		targetnode = makeNode(ResTarget);
		colref = makeNode(ColumnRef);
		colref->fields = list_make1(makeNode(A_Star));
		targetnode->val = (Node *) colref;
		tailstmt->targetList = list_make1(targetnode);
		tailstmt->fromClause = list_make1(rangefunc);
	}

	/* make finalStmt */
	finalStmt = (SelectStmt *)makeNode(SelectStmt);
	/* make select *  ....*/
	{
		node = makeColumnRef(finalname, list_make1(makeNode(A_Star)));
		targetnode = makeNode(ResTarget);
		targetnode->val = node;
		finalStmt->targetList = list_make1(targetnode);
	}

	/* make connect_by_isleaf col */
	if (hasisleaf)
	{
		/* make lead(final_level__query.level) over() */
		WindowDef *n = makeNode(WindowDef);

		n->frameOptions = FRAMEOPTION_DEFAULTS;

		node = makeColumnRef(finalname, list_make1(makeString(levelcol)));
		leadfun = makeNode(FuncCall);
		leadfun->funcname = list_make1(makeString(pstrdup("lead")));
		leadfun->args = list_make1(node);
		leadfun->over = n;

		/* make CASE WHEN lead(final_level__query.level) over() > final_level__query.level THEN 0 ELSE 1 END */
		caseisleaf = makeNode(CaseExpr);
		casewhenisleaf = makeNode(CaseWhen);
		casewhenisleaf->expr = (Expr *)makeSimpleA_Expr(AEXPR_OP, ">", (Node *)leadfun, (Node *)node, -1);
		casewhenisleaf->result = (Expr *)makeIntConst(0);

		caseisleaf_args = lappend(caseisleaf_args, casewhenisleaf);
		caseisleaf->args = caseisleaf_args;
		caseisleaf->defresult = (Expr *)makeIntConst(1);

		isleaf = makeNode(ResTarget);
		isleaf->name = isleafcol;
		isleaf->val = (Node *)caseisleaf;
		finalStmt->targetList = lappend(finalStmt->targetList, isleaf);
	}

	/* move tailstmt into new sub select */
	finalSubselect = (RangeSubselect *)makeNode(RangeSubselect);
	finalAlias = (Alias *)makeNode(Alias);
	finalAlias->aliasname = finalname;
	finalSubselect->alias = (Alias *)finalAlias;
	finalSubselect->subquery = (Node *)tailstmt;
	finalStmt->fromClause = list_make1(finalSubselect);

	/* handle connect by rownum */
	if (connect_by_flag & CONNECT_BY_CONTAIN_ROWNUM &&
		!has_no_special_cols)
	{
		A_Expr		*aexpr;

		tab_ctx.tbname = finalname;

		finalStmt->whereClause = connect_by_rownum;
		finalStmt->whereClause = parsetree_tree_walker(finalStmt->whereClause,
			add_tabreference_walker, (void *) &tab_ctx);

		tab_ctx.tbname = internel_priorname;

		/*
		 * maye origin_connect_by has prior operator which
		 * will delete rows which level is 1 because its prior
		 * colname is null
		 * so we add level = 1 clause to include rows which'level is
		 * 1
		 */
		aexpr = makeSimpleA_Expr(AEXPR_OP, "=", makeColumnRef(levelcol, NIL),
						makeIntConst(1), -1);
		finalStmt->whereClause = makeOrExpr((Node *) aexpr, finalStmt->whereClause, -1);
	}

	finalSubselect2 = (RangeSubselect *) makeNode(RangeSubselect);
	finalAlias = (Alias *) makeNode(Alias);
	finalAlias->aliasname = allfinalname;
	finalSubselect2->alias = (Alias *) finalAlias;
	finalSubselect2->subquery = (Node *) finalStmt;

	allfinalstmt = makeNode(SelectStmt);
	allfinalstmt->fromClause = list_make1(finalSubselect2);

	tab_ctx.tbname = allfinalname;
	/* Begin - BUG#Z00005447 */
	tab_ctx.only_replace = true;
	/* End - BUG#Z00005447 */

	allfinaltarget = (List *) parsetree_tree_walker((Node *) allfinaltarget,
			add_tabreference_walker, (void *) &tab_ctx);

	allfinalstmt->targetList = (List *) parsetree_tree_walker((Node *) allfinaltarget,
		replace_pseduocol_walker, &pseudocol_ctx);

	/* remove schema to allfinalname */
	whereClause = parsetree_tree_walker(whereClause,
			add_tabreference_walker, (void *) &tab_ctx);
	allfinalstmt->whereClause = whereClause;

	/* Begin - BUG#Z00005447 */
	/*
	 * if there is order by clause in connect by query
	 * siblings_by has been handle by window func row_num() over(order by)
	 *
	 * common order by maybe reference a.xx
	 */
	if (sortClause && !siblings_by)
	{
		allfinalstmt->sortClause = (List *) parsetree_tree_walker((Node *) sortClause,
					add_tabreference_walker, (void *) &tab_ctx);
	}
	tab_ctx.tbname = internel_priorname;
	/* End - BUG#Z00005447 */

	allfinalstmt->withClause = withclause;
	allfinalstmt->limitCount = limitCount;
	allfinalstmt->limitOffset = limitOffset;
	allfinalstmt->lockingClause = lockingClause;
	allfinalstmt->groupClause = groupClause;
	allfinalstmt->windowClause = windowClause;
	allfinalstmt->havingClause = havingClause;
	/* Begin - bug#00008247 */
	allfinalstmt->intoClause = intoClause;
	allfinalstmt->distinctClause = distinctClause;
	/* End - bug#00008247 */

	free_parsestate(cur_pstate);

	list_free(prior_colname_list);
	list_free(rootlist);
	list_free(syspathlist);

	return allfinalstmt;
}


/*
 * checkLevelQueryName
 */
bool
checkLevelQueryName(IgnoreStr *name, char *label)
{
	if ((0 == strcmp(label, name->lq_table)) ||
		(0 == strcmp(label, name->lq_level)) ||
		(0 == strcmp(label, name->lq_isleaf)) ||
		(name->lq_arrpath && (0 == strcmp(name->lq_arrpath, label))) ||
		(name->lq_connect_by_iscycle && (0 == strcmp(name->lq_connect_by_iscycle, label))))
	{
		return true;
	}
	else if (name->lq_path != NIL ||
			name->lq_root != NIL ||
			name->lq_prior != NIL ||
			name->lq_table_col != NIL)
	{
		ListCell *lc;
		bool match = false;
		List *tmplist;

		tmplist = list_make4(name->lq_path, name->lq_root, name->lq_prior, name->lq_table_col);
		foreach (lc, tmplist)
		{
			List *lq = (List *) lfirst(lc);
			ListCell *lcc;

			foreach (lcc, lq)
			{
				Value *v = (Value *) lfirst(lcc);

				if (0 == strcmp(v->val.str, label))
				{
					match = true;
					break;
				}
			}
			if (match)
				break;
		}
		if (match)
			return true;
	}

	return false;
}

