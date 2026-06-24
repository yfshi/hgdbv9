/*-------------------------------------------------------------------------
 *
 * parse_node.h
 *		Internal definitions for parser
 *
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/parser/parse_node.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PARSE_NODE_H
#define PARSE_NODE_H

#include "nodes/parsenodes.h"
#include "utils/queryenvironment.h"
#include "utils/relcache.h"


/* Forward references for some structs declared below */
typedef struct ParseState ParseState;
typedef struct ParseNamespaceItem ParseNamespaceItem;
typedef struct ParseNamespaceColumn ParseNamespaceColumn;

/*
 * Expression kinds distinguished by transformExpr().  Many of these are not
 * semantically distinct so far as expression transformation goes; rather,
 * we distinguish them so that context-specific error messages can be printed.
 *
 * Note: EXPR_KIND_OTHER is not used in the core code, but is left for use
 * by extension code that might need to call transformExpr().  The core code
 * will not enforce any context-driven restrictions on EXPR_KIND_OTHER
 * expressions, so the caller would have to check for sub-selects, aggregates,
 * window functions, SRFs, etc if those need to be disallowed.
 */
typedef enum ParseExprKind
{
	EXPR_KIND_NONE = 0,			/* "not in an expression" */
	EXPR_KIND_OTHER,			/* reserved for extensions */
	EXPR_KIND_JOIN_ON,			/* JOIN ON */
	EXPR_KIND_JOIN_USING,		/* JOIN USING */
	EXPR_KIND_FROM_SUBSELECT,	/* sub-SELECT in FROM clause */
	EXPR_KIND_FROM_FUNCTION,	/* function in FROM clause */
	EXPR_KIND_WHERE,			/* WHERE */
	EXPR_KIND_HAVING,			/* HAVING */
	EXPR_KIND_FILTER,			/* FILTER */
	EXPR_KIND_WINDOW_PARTITION, /* window definition PARTITION BY */
	EXPR_KIND_WINDOW_ORDER,		/* window definition ORDER BY */
	EXPR_KIND_WINDOW_FRAME_RANGE,	/* window frame clause with RANGE */
	EXPR_KIND_WINDOW_FRAME_ROWS,	/* window frame clause with ROWS */
	EXPR_KIND_WINDOW_FRAME_GROUPS,	/* window frame clause with GROUPS */
	EXPR_KIND_SELECT_TARGET,	/* SELECT target list item */
	EXPR_KIND_INSERT_TARGET,	/* INSERT target list item */
	EXPR_KIND_UPDATE_SOURCE,	/* UPDATE assignment source item */
	EXPR_KIND_UPDATE_TARGET,	/* UPDATE assignment target item */
	EXPR_KIND_MERGE_WHEN,		/* MERGE WHEN [NOT] MATCHED condition */
	EXPR_KIND_GROUP_BY,			/* GROUP BY */
	EXPR_KIND_ORDER_BY,			/* ORDER BY */
	EXPR_KIND_DISTINCT_ON,		/* DISTINCT ON */
	EXPR_KIND_LIMIT,			/* LIMIT */
	EXPR_KIND_OFFSET,			/* OFFSET */
	EXPR_KIND_RETURNING,		/* RETURNING */
	EXPR_KIND_VALUES,			/* VALUES */
	EXPR_KIND_VALUES_SINGLE,	/* single-row VALUES (in INSERT only) */
	EXPR_KIND_CHECK_CONSTRAINT, /* CHECK constraint for a table */
	EXPR_KIND_DOMAIN_CHECK,		/* CHECK constraint for a domain */
	EXPR_KIND_COLUMN_DEFAULT,	/* default value for a table column */
	EXPR_KIND_FUNCTION_DEFAULT, /* default parameter value for function */
	EXPR_KIND_INDEX_EXPRESSION, /* index expression */
	EXPR_KIND_INDEX_PREDICATE,	/* index predicate */
	EXPR_KIND_STATS_EXPRESSION, /* extended statistics expression */
	EXPR_KIND_ALTER_COL_TRANSFORM,	/* transform expr in ALTER COLUMN TYPE */
	EXPR_KIND_EXECUTE_PARAMETER,	/* parameter value in EXECUTE */
	EXPR_KIND_TRIGGER_WHEN,		/* WHEN condition in CREATE TRIGGER */
	EXPR_KIND_POLICY,			/* USING or WITH CHECK expr in policy */
	EXPR_KIND_PARTITION_BOUND,	/* partition bound expression */
	EXPR_KIND_PARTITION_EXPRESSION, /* PARTITION BY expression */
	EXPR_KIND_CALL_ARGUMENT,	/* procedure argument in CALL */
	EXPR_KIND_COPY_WHERE,		/* WHERE condition in COPY FROM */
	EXPR_KIND_GENERATED_COLUMN, /* generation expression for a column */
	EXPR_KIND_CYCLE_MARK,		/* cycle mark value */
	EXPR_KIND_GROUP_BY_DISTINCT,	/* Req: SRS-SQL-NLSSORT, used to distinguish `DISTINCT xx ORDER BY` xx when do pseudo collation replace */
	EXPR_KIND_ASOF,				/* ReqID:SRS-RECOVERY-FLASHBACK */
	EXPR_KIND_VERSIONS_BETWEEN,	/* ReqID:SRS-RECOVERY-FLASHBACK */
	EXPR_KIND_HAVING_FUNCTION,	/* Req: SRS-MYSQL-SQL-Select, The function located in the having clause */
} ParseExprKind;


/* Begin - SRS-SQL-MULTITABLEINSERT */
/*
 * parse stmt kind for special
 * like insert all/first subquery doesn't allow
 * sequence
 */
typedef enum ParseStmtKind
{
	PARSE_KIND_NONE = 0,
	PARSE_KIND_MULTITABLE_INSERT, /* insert all/first stmt */
} ParseStmtKind;
/* End - SRS-SQL-MULTITABLEINSERT */



/*
 * Function signatures for parser hooks
 */
typedef Node *(*PreParseColumnRefHook) (ParseState *pstate, ColumnRef *cref);
typedef Node *(*PostParseColumnRefHook) (ParseState *pstate, ColumnRef *cref, Node *var);
typedef Node *(*ParseParamRefHook) (ParseState *pstate, ParamRef *pref);
typedef Node *(*CoerceParamHook) (ParseState *pstate, Param *param,
								  Oid targetTypeId, int32 targetTypeMod,
								  int location);

/* Begin - ReqID:SRS-PLSQL-Collection */
typedef Node *(*ParseIndirectionHook)(ParseState *pstate,
				Node *ind_arg,
				const char *targetName,
				List *indirection,
				Node *rhs,
				int location);
/* End - ReqID:SRS-PLSQL-Collection */

/* Begin - SRS-PLSQL-SUBPROC */
typedef int (*ParseSubprocFuncHook) (ParseState *pstate, List *funcname,
				List **fargs, /* return value */
				List *fargnames,
				int nargs,
				Oid *argtypes,
				bool expand_variadic,
				bool expand_defaults,
				bool proc_call,
				Oid *funcid,	/* return value */
				Oid *rettype,	/* return value */
				bool *retset,	/* return value */
				int *nvargs,	/* return value */
				Oid *vatype,	/* return value */
				Oid **true_typeids, /* return value */
				List **argdefaults, /* return value */
				void **pfunc,		/* return value */
				/* Begin - ReqID:SRS-PLSQL-RECORD */
				bool has_udt_type);	/* argument have Oracle record type or other udt type? */
				/* End - ReqID:SRS-PLSQL-RECORD */

/* End - SRS-PLSQL-SUBPROC */
/* Begin - ReqID:SRS-PLSQL-RECORD */
typedef Node * (*ParseOraRecordConstructHook) (ParseState *pstate, List *recname, List *fargs,
				  FuncCall *fn, int location);
/* End - ReqID:SRS-PLSQL-RECORD */

/* Begin - ReqID:SRS-PLSQL-CURSOR */
typedef TupleDesc (*ParseParamTupleDescHook) (ParseState *pstate, Param *param);
/* End - ReqID:SRS-PLSQL-CURSOR */

/* Begin - ReqID:SRS-SQL-SYNONYM */
typedef Oid (*SynnameGetRelname_hook_type)(const char *synname,
													const char *synnsp,
													char **objschema,
													char **objname,
													char **objdblnksrv,
													List **synoids);
typedef bool (*SynnameGetFuncInfo_hook_type)(const char *synname,const char *synnsp, char **funcname,char **funcnsp, List **synoids);
typedef Oid (*SynnameGetPkgoid_recursive_hook_type)(const char *synname, Oid nspkg, List **synoids);
typedef void (*SynIdGetname_hook_type)(Oid synid, char **synname, char **synnsp);

extern PGDLLIMPORT SynnameGetRelname_hook_type SynnameGetRelname_hook;
extern PGDLLIMPORT SynnameGetFuncInfo_hook_type SynnameGetFuncInfo_hook;
extern PGDLLIMPORT SynnameGetPkgoid_recursive_hook_type SynnameGetPkgoid_recursive_hook;
extern PGDLLIMPORT SynIdGetname_hook_type SynIdGetname_hook;
/* End - ReqID:SRS-SQL-SYNONYM */


/*
 * State information used during parse analysis
 *
 * parentParseState: NULL in a top-level ParseState.  When parsing a subquery,
 * links to current parse state of outer query.
 *
 * p_sourcetext: source string that generated the raw parsetree being
 * analyzed, or NULL if not available.  (The string is used only to
 * generate cursor positions in error messages: we need it to convert
 * byte-wise locations in parse structures to character-wise cursor
 * positions.)
 *
 * p_rtable: list of RTEs that will become the rangetable of the query.
 * Note that neither relname nor refname of these entries are necessarily
 * unique; searching the rtable by name is a bad idea.
 *
 * p_joinexprs: list of JoinExpr nodes associated with p_rtable entries.
 * This is one-for-one with p_rtable, but contains NULLs for non-join
 * RTEs, and may be shorter than p_rtable if the last RTE(s) aren't joins.
 *
 * p_joinlist: list of join items (RangeTblRef and JoinExpr nodes) that
 * will become the fromlist of the query's top-level FromExpr node.
 *
 * p_namespace: list of ParseNamespaceItems that represents the current
 * namespace for table and column lookup.  (The RTEs listed here may be just
 * a subset of the whole rtable.  See ParseNamespaceItem comments below.)
 *
 * p_lateral_active: true if we are currently parsing a LATERAL subexpression
 * of this parse level.  This makes p_lateral_only namespace items visible,
 * whereas they are not visible when p_lateral_active is FALSE.
 *
 * p_ctenamespace: list of CommonTableExprs (WITH items) that are visible
 * at the moment.  This is entirely different from p_namespace because a CTE
 * is not an RTE, rather "visibility" means you could make an RTE from it.
 *
 * p_future_ctes: list of CommonTableExprs (WITH items) that are not yet
 * visible due to scope rules.  This is used to help improve error messages.
 *
 * p_parent_cte: CommonTableExpr that immediately contains the current query,
 * if any.
 *
 * p_target_relation: target relation, if query is INSERT/UPDATE/DELETE/MERGE
 *
 * p_target_nsitem: target relation's ParseNamespaceItem.
 *
 * p_is_insert: true to process assignment expressions like INSERT, false
 * to process them like UPDATE.  (Note this can change intra-statement, for
 * cases like INSERT ON CONFLICT UPDATE.)
 *
 * p_windowdefs: list of WindowDefs representing WINDOW and OVER clauses.
 * We collect these while transforming expressions and then transform them
 * afterwards (so that any resjunk tlist items needed for the sort/group
 * clauses end up at the end of the query tlist).  A WindowDef's location in
 * this list, counting from 1, is the winref number to use to reference it.
 *
 * p_expr_kind: kind of expression we're currently parsing, as per enum above;
 * EXPR_KIND_NONE when not in an expression.
 *
 * p_next_resno: next TargetEntry.resno to assign, starting from 1.
 *
 * p_multiassign_exprs: partially-processed MultiAssignRef source expressions.
 *
 * p_locking_clause: query's FOR UPDATE/FOR SHARE clause, if any.
 *
 * p_locked_from_parent: true if parent query level applies FOR UPDATE/SHARE
 * to this subquery as a whole.
 *
 * p_resolve_unknowns: resolve unknown-type SELECT output columns as type TEXT
 * (this is true by default).
 *
 * p_hasAggs, p_hasWindowFuncs, etc: true if we've found any of the indicated
 * constructs in the query.
 *
 * p_last_srf: the set-returning FuncExpr or OpExpr most recently found in
 * the query, or NULL if none.
 *
 * p_pre_columnref_hook, etc: optional parser hook functions for modifying the
 * interpretation of ColumnRefs and ParamRefs.
 *
 * p_ref_hook_state: passthrough state for the parser hook functions.
 */
struct ParseState
{
	ParseState *parentParseState;	/* stack link */
	const char *p_sourcetext;	/* source text, or NULL if not available */
	List	   *p_rtable;		/* range table so far */
	List	   *p_joinexprs;	/* JoinExprs for RTE_JOIN p_rtable entries */
	List	   *p_joinlist;		/* join items so far (will become FromExpr
								 * node's fromlist) */
	List	   *p_namespace;	/* currently-referenceable RTEs (List of
								 * ParseNamespaceItem) */
	bool		p_lateral_active;	/* p_lateral_only items visible? */
	List	   *p_ctenamespace; /* current namespace for common table exprs */
	List	   *p_future_ctes;	/* common table exprs not yet in namespace */
	CommonTableExpr *p_parent_cte;	/* this query's containing CTE */
	Relation	p_target_relation;	/* INSERT/UPDATE/DELETE/MERGE target rel */
	ParseNamespaceItem *p_target_nsitem;	/* target rel's NSItem, or NULL */
	bool		p_is_insert;	/* process assignment like INSERT not UPDATE */
	List	   *p_windowdefs;	/* raw representations of window clauses */
	ParseExprKind p_expr_kind;	/* what kind of expression we're parsing */
	int			p_next_resno;	/* next targetlist resno to assign */
	List	   *p_multiassign_exprs;	/* junk tlist entries for multiassign */
	List	   *p_locking_clause;	/* raw FOR UPDATE/FOR SHARE info */
	bool		p_locked_from_parent;	/* parent has marked this subquery
										 * with FOR UPDATE/FOR SHARE */
	bool		p_resolve_unknowns; /* resolve unknown-type SELECT outputs as
									 * type text */

	QueryEnvironment *p_queryEnv;	/* curr env, incl refs to enclosing env */

	/* Flags telling about things found in the query: */
	bool		p_hasAggs;
	bool		p_hasWindowFuncs;
	bool		p_hasTargetSRFs;
	bool		p_hasSubLinks;
	bool		p_hasModifyingCTE;
	bool		p_hasRownum;	/* ReqID:SRS-SQL-ROWNUM */

	Node	   *p_last_srf;		/* most recent set-returning func/op found */

	/*
	 * Optional hook functions for parser callbacks.  These are null unless
	 * set up by the caller of make_parsestate.
	 */
	PreParseColumnRefHook p_pre_columnref_hook;
	PostParseColumnRefHook p_post_columnref_hook;
	ParseParamRefHook p_paramref_hook;
	CoerceParamHook p_coerce_param_hook;
	/* Begin - SRS-PLSQL-SUBPROC */
	ParseSubprocFuncHook p_subprocfunc_hook;
	/* End - SRS-PLSQL-SUBPROC */
	/* Begin - ReqID:SRS-PLSQL-CURSOR */
	ParseParamTupleDescHook p_param_tupdesc_hook;
	/* End - ReqID:SRS-PLSQL-CURSOR */
	/* Begin - ReqID:SRS-PLSQL-RECORD */
	ParseOraRecordConstructHook p_orarecordconstruct_hook;
	/* End - ReqID:SRS-PLSQL-RECORD */
	void	   *p_ref_hook_state;	/* common passthrough link for above */
	/* Begin - Bug#Z167 */
	uint16	    merge_on_attr_size;
	uint8 	   *merge_on_attrno;
	/* End - Bug#Z167 */
	/* Begin - ReqID:SRS-SQL-FUNCOUT */
	bool 		p_isVarParamState;
	/* End - ReqID:SRS-SQL-FUNCOUT */
	bool		do_from_call;	/* ReqID:SRS-SQL-CALL */
	List	   **p_targetlist;	/* BUG#M0000217 */
	List	   *p_group_clauses;	/* ReqID:SRS-MYSQL-SQL-Select */

	/* Begin - ReqID:SRS-SQL-OUTERJOIN(+) */
	/* For (+) outer join */
	bool	p_hasAnsiJoinType;
	bool	p_hasOraJoinType;
	/* End - ReqID:SRS-SQL-OUTERJOIN(+) */
	/* Begin - BUG#M0000376 */
	bool	p_not_set_oraparammode;	/* when tranform a FunCall, don't cosider the placeholder mode */
	char	p_oraparammode;			/* the argument mode of a function or procedure */
	bool	p_not_transform_funcCall;	/* don't transform funcCall */
	/* End - BUG#M0000376 */
	/* Begin - ReqID:SRS-MYSQL-UPDATE */
	List	   *p_updateRelations;
	bool		p_addUpdateTable;
	List	   *p_target_rels;
	List	   *p_updateRangeVars;	/* For multiple-update, use relationClase to generate RangeVar list. */
	List	   *p_target_rangetblentry;
	bool		p_isupdate;
	List	   *p_indexhintLists;
	/* End - ReqID:SRS-MYSQL-UPDATE */
	/* Begin - ReqID:SRS-MYSQL-SQL-INSERT */
	List	   *p_varnamespace;
	bool		p_ruleflag;
	bool		p_isexistVar;
	/* End - ReqID:SRS-MYSQL-SQL-INSERT */
	/* Begin - ReqID:SRS-MYSQL-SQL-DELETE */
	bool		p_addDeleteTable;
	List	   *p_delrellist;
	List	   *p_deleteRangerVars;
	List	   *p_addupdatedeleterv;
	/* End - ReqID:SRS-MYSQL-SQL-DELETE */
	bool		p_isreplace; /* BUG#Z1626 */

	/* Begin - ReqID:SRS-MYSQL-DATETYPE */
	bool		p_createastmt;
	/* End - ReqID:SRS-MYSQL-DATETYPE */

	/* Begin - ReqID:SRS-SQL-HierarchicalQueries */
	IgnoreStr  *p_connectby_names;
	bool		p_check_connectby;
	/* End - ReqID:SRS-SQL-HierarchicalQueries */

	/* ReqID:SRS-MYSQL-SET
	 * a flag for user define variable get value,
	 * if set @var, get current variable value,
	 * else return VarRef node. BUG#Z6860.
	 * if p_myssetvar is true, represent
	 * it is a @var = @vara + 1
	 */
	bool		p_myssetvar;
	/*
	 * If the value is set to true, it means
	 * VarRef node has been parsed.
	 */
	bool		p_hasuservar;
	/* ReqID:SRS-MYSQL-SET */

	/** SRS-MYSQL-HEX-LITERAL
	 * If column's default value need cast to target type, 
	 * we set p_cook_default to true to set 4th 'is_insert' flag
	 * in cast function.
	 */
	bool		p_cook_default;

	/** SRS-MYSQL-HEX-LITERAL
	 * Mark there is alter column type context.
	 * When alter column type, such as alter text to int,
	 * cast function will be called in INSERT context.
	 */
	bool		p_alter_column_type;

	/** SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT
	 * whether to check the column of type in INSERT context.
	 * true: check and add funcexpr node
	 */
	bool		p_check_column_auto_increment;

	/* Begin - ReqID:SRS-SECURITY-DATADESENSITIZATION
	 * pstate->p_subselect record where the row is
	 * in a subquery, which is needed for data masking.
	 */
	bool		p_subselect;
	/* End - ReqID:SRS-SECURITY-DATADESENSITIZATION */

	/* Begin - ReqID:SRS-PLSQL-Collection */
	ParseIndirectionHook	p_indirection_hook;
	/* End - ReqID:SRS-PLSQL-Collection */
	/* Begin - SRS-SQL-MULTITABLEINSERT */
	ParseStmtKind p_stmt_kind;
	/* End - SRS-SQL-MULTITABLEINSERT */

	/* Begin - ReqID:SRS-MYSQL-SQL-LOADDATA */
	bool	mys_load_local;		/* mark its load local case on mysql mode */
	bool	select_into;		/* mark its select into */
	bool	is_load_data;		/* mark its load data, bug#z7537 */
	/* End - ReqID:SRS-MYSQL-SQL-LOADDATA */
	/* Begin - ReqID:SRS-SECURITY-FLASHBACK */
	bool	p_tryVersionsPseudoColumn;
	/* End - ReqID:SRS-SECURITY-FLASHBACK */
	/* Begin - OA#00002227 */
	char	*p_pkgname;		/* transform package function which has default value */
	/* End - OA#00002227*/
};

/*
 * An element of a namespace list.
 *
 * p_names contains the table name and column names exposed by this nsitem.
 * (Typically it's equal to p_rte->eref, but for a JOIN USING alias it's
 * equal to p_rte->join_using_alias.  Since the USING columns will be the
 * join's first N columns, the net effect is just that we expose only those
 * join columns via this nsitem.)
 *
 * p_rte and p_rtindex link to the underlying rangetable entry.
 *
 * The p_nscolumns array contains info showing how to construct Vars
 * referencing the names appearing in the p_names->colnames list.
 *
 * Namespace items with p_rel_visible set define which RTEs are accessible by
 * qualified names, while those with p_cols_visible set define which RTEs are
 * accessible by unqualified names.  These sets are different because a JOIN
 * without an alias does not hide the contained tables (so they must be
 * visible for qualified references) but it does hide their columns
 * (unqualified references to the columns refer to the JOIN, not the member
 * tables, so we must not complain that such a reference is ambiguous).
 * Various special RTEs such as NEW/OLD for rules may also appear with only
 * one flag set.
 *
 * While processing the FROM clause, namespace items may appear with
 * p_lateral_only set, meaning they are visible only to LATERAL
 * subexpressions.  (The pstate's p_lateral_active flag tells whether we are
 * inside such a subexpression at the moment.)	If p_lateral_ok is not set,
 * it's an error to actually use such a namespace item.  One might think it
 * would be better to just exclude such items from visibility, but the wording
 * of SQL:2008 requires us to do it this way.  We also use p_lateral_ok to
 * forbid LATERAL references to an UPDATE/DELETE target table.
 *
 * At no time should a namespace list contain two entries that conflict
 * according to the rules in checkNameSpaceConflicts; but note that those
 * are more complicated than "must have different alias names", so in practice
 * code searching a namespace list has to check for ambiguous references.
 */
struct ParseNamespaceItem
{
	Alias	   *p_names;		/* Table and column names */
	RangeTblEntry *p_rte;		/* The relation's rangetable entry */
	int			p_rtindex;		/* The relation's index in the rangetable */
	/* array of same length as p_names->colnames: */
	ParseNamespaceColumn *p_nscolumns;	/* per-column data */
	bool		p_rel_visible;	/* Relation name is visible? */
	bool		p_cols_visible; /* Column names visible as unqualified refs? */
	bool		p_lateral_only; /* Is only visible to LATERAL expressions? */
	bool		p_lateral_ok;	/* If so, does join type allow use? */
};

/*
 * Data about one column of a ParseNamespaceItem.
 *
 * We track the info needed to construct a Var referencing the column
 * (but only for user-defined columns; system column references and
 * whole-row references are handled separately).
 *
 * p_varno and p_varattno identify the semantic referent, which is a
 * base-relation column unless the reference is to a join USING column that
 * isn't semantically equivalent to either join input column (because it is a
 * FULL join or the input column requires a type coercion).  In those cases
 * p_varno and p_varattno refer to the JOIN RTE.
 *
 * p_varnosyn and p_varattnosyn are either identical to p_varno/p_varattno,
 * or they specify the column's position in an aliased JOIN RTE that hides
 * the semantic referent RTE's refname.  (That could be either the JOIN RTE
 * in which this ParseNamespaceColumn entry exists, or some lower join level.)
 *
 * If an RTE contains a dropped column, its ParseNamespaceColumn struct
 * is all-zeroes.  (Conventionally, test for p_varno == 0 to detect this.)
 */
struct ParseNamespaceColumn
{
	Index		p_varno;		/* rangetable index */
	AttrNumber	p_varattno;		/* attribute number of the column */
	Oid			p_vartype;		/* pg_type OID */
	int32		p_vartypmod;	/* type modifier value */
	Oid			p_varcollid;	/* OID of collation, or InvalidOid */
	Index		p_varnosyn;		/* rangetable index of syntactic referent */
	AttrNumber	p_varattnosyn;	/* attribute number of syntactic referent */
	bool		p_dontexpand;	/* not included in star expansion */
	bool		p_var_visible;  /* Mark whether the column is visible */	/* ReqID:SRS-SQL-INVISIBLECOL */
	/* Begin - BUG#8430 */
	bool		union_null_res;
	/* End - BUG#8430 */
};

/* Support for parser_errposition_callback function */
typedef struct ParseCallbackState
{
	ParseState *pstate;
	int			location;
	ErrorContextCallback errcallback;
} ParseCallbackState;


extern ParseState *make_parsestate(ParseState *parentParseState);
extern void free_parsestate(ParseState *pstate);
extern int	parser_errposition(ParseState *pstate, int location);

extern void setup_parser_errposition_callback(ParseCallbackState *pcbstate,
											  ParseState *pstate, int location);
extern void cancel_parser_errposition_callback(ParseCallbackState *pcbstate);

extern void transformContainerType(Oid *containerType, int32 *containerTypmod);

extern SubscriptingRef *transformContainerSubscripts(ParseState *pstate,
													 Node *containerBase,
													 Oid containerType,
													 int32 containerTypMod,
													 List *indirection,
													 bool isAssignment);

extern Const *make_const(ParseState *pstate, Value *value, int location);
/* Begin - ReqID:SRS-SQL-OUTERJOIN(+) */
#define hasPlusFlag(expr) \
	   (((nodeTag((expr)) == T_JoinExpr && ((JoinExpr *)(expr))->has_outer_join) || \
	   (nodeTag((expr)) == T_OpExpr && ((OpExpr *)(expr))->has_outer_join) || \
	   (nodeTag((expr)) == T_BoolExpr && ((BoolExpr *)(expr))->has_outer_join))|| \
	   (nodeTag((expr)) == T_Var && ((Var *)(expr))->has_outer_join) ||	\
	   (nodeTag((expr)) == T_NullTest && nodeTag(((NullTest *)expr)->arg) == T_Var && ((Var *)((NullTest *)expr)->arg)->has_outer_join) || \
	   (nodeTag((expr)) == T_NullTest && nodeTag(((NullTest *)expr)->arg) == T_OpExpr && ((OpExpr *)((NullTest *)expr)->arg)->has_outer_join) || \
	   (nodeTag((expr)) == T_FuncExpr && ((FuncExpr *)(expr))->has_outer_join) || \
	   (nodeTag((expr)) == T_Aggref && ((Aggref *)(expr))->has_outer_join))
/* End - ReqID:SRS-SQL-OUTERJOIN(+) */

/* Begin BUG#5048 5070 */
typedef Datum (* mysql_hexstring_parse_hook_type) (const char *str);
extern PGDLLIMPORT mysql_hexstring_parse_hook_type mysql_hexstring_parse_hook;

typedef Datum (* mysql_bitstring_parse_hook_type) (const char *str);
extern PGDLLIMPORT mysql_bitstring_parse_hook_type mysql_bitstring_parse_hook;
/* End BUG#5048 5070 */
#endif							/* PARSE_NODE_H */
