/*-------------------------------------------------------------------------
 *
 * plmysql.h		- Definitions for the PL/mySQL
 *			  procedural language
 *
 * Portions Copyright (c)  2023, IvorySQL
 *
 *
 * IDENTIFICATION
 *	  src/pl/plmysql/src/plmysql.h
 *
 *add the file for requirement "SRS-MISC-MYSQL-FRAME"
 *
 *-------------------------------------------------------------------------
 */

#ifndef PLMYSQL_H
#define PLMYSQL_H

#include "access/xact.h"
#include "commands/event_trigger.h"
#include "commands/trigger.h"
#include "executor/spi.h"
#include "utils/expandedrecord.h"
#include "utils/typcache.h"


/**********************************************************************
 * Definitions
 **********************************************************************/

/* define our text domain for translations */
#undef TEXTDOMAIN
#define TEXTDOMAIN PG_TEXTDOMAIN("plmysql")

#undef _
#define _(x) dgettext(TEXTDOMAIN, x)

/*
 * Compiler's namespace item types
 */
typedef enum PLmySQL_nsitem_type
{
	PLMYSQL_NSTYPE_LABEL,		/* block label */
	PLMYSQL_NSTYPE_VAR,			/* scalar variable */
	PLMYSQL_NSTYPE_REC			/* composite variable */
} PLmySQL_nsitem_type;

/*
 * A PLMYSQL_NSTYPE_LABEL stack entry must be one of these types
 */
typedef enum PLmySQL_label_type
{
	PLMYSQL_LABEL_BLOCK,		/* DECLARE/BEGIN block */
	PLMYSQL_LABEL_LOOP,			/* looping construct */
	PLMYSQL_LABEL_OTHER			/* anything else */
} PLmySQL_label_type;

/*
 * Datum array node types
 */
typedef enum PLmySQL_datum_type
{
	PLMYSQL_DTYPE_VAR,
	PLMYSQL_DTYPE_ROW,
	PLMYSQL_DTYPE_REC,
	PLMYSQL_DTYPE_RECFIELD,
	PLMYSQL_DTYPE_PROMISE
} PLmySQL_datum_type;

/*
 * DTYPE_PROMISE datums have these possible ways of computing the promise
 */
typedef enum PLmySQL_promise_type
{
	PLMYSQL_PROMISE_NONE = 0,	/* not a promise, or promise satisfied */
	PLMYSQL_PROMISE_TG_NAME,
	PLMYSQL_PROMISE_TG_WHEN,
	PLMYSQL_PROMISE_TG_LEVEL,
	PLMYSQL_PROMISE_TG_OP,
	PLMYSQL_PROMISE_TG_RELID,
	PLMYSQL_PROMISE_TG_TABLE_NAME,
	PLMYSQL_PROMISE_TG_TABLE_SCHEMA,
	PLMYSQL_PROMISE_TG_NARGS,
	PLMYSQL_PROMISE_TG_ARGV,
	PLMYSQL_PROMISE_TG_EVENT,
	PLMYSQL_PROMISE_TG_TAG
} PLmySQL_promise_type;

/*
 * Variants distinguished in PLmySQL_type structs
 */
typedef enum PLmySQL_type_type
{
	PLMYSQL_TTYPE_SCALAR,		/* scalar types and domains */
	PLMYSQL_TTYPE_REC,			/* composite types, including RECORD */
	PLMYSQL_TTYPE_PSEUDO		/* pseudotypes */
} PLmySQL_type_type;

/*
 * Execution tree node types
 */
typedef enum PLmySQL_stmt_type
{
	PLMYSQL_STMT_BLOCK,
	PLMYSQL_STMT_ASSIGN,
	PLMYSQL_STMT_IF,
	PLMYSQL_STMT_CASE,
	PLMYSQL_STMT_LOOP,
	PLMYSQL_STMT_WHILE,
	PLMYSQL_STMT_FORI,
	PLMYSQL_STMT_FORS,
	PLMYSQL_STMT_FORC,
	PLMYSQL_STMT_FOREACH_A,
	PLMYSQL_STMT_EXIT,
	PLMYSQL_STMT_RETURN,
	PLMYSQL_STMT_RETURN_NEXT,
	PLMYSQL_STMT_RETURN_QUERY,
	PLMYSQL_STMT_RAISE,
	PLMYSQL_STMT_ASSERT,
	PLMYSQL_STMT_EXECSQL,
	PLMYSQL_STMT_DYNEXECUTE,
	PLMYSQL_STMT_DYNFORS,
	PLMYSQL_STMT_GETDIAG,
	PLMYSQL_STMT_OPEN,
	PLMYSQL_STMT_FETCH,
	PLMYSQL_STMT_CLOSE,
	PLMYSQL_STMT_PERFORM,
	PLMYSQL_STMT_CALL,
	PLMYSQL_STMT_COMMIT,
	PLMYSQL_STMT_ROLLBACK
} PLmySQL_stmt_type;

/*
 * Execution node return codes
 */
enum
{
	PLMYSQL_RC_OK,
	PLMYSQL_RC_EXIT,
	PLMYSQL_RC_RETURN,
	PLMYSQL_RC_CONTINUE
};

/*
 * GET DIAGNOSTICS information items
 */
typedef enum PLmySQL_getdiag_kind
{
	PLMYSQL_GETDIAG_ROW_COUNT,
	PLMYSQL_GETDIAG_CONTEXT,
	PLMYSQL_GETDIAG_ERROR_CONTEXT,
	PLMYSQL_GETDIAG_ERROR_DETAIL,
	PLMYSQL_GETDIAG_ERROR_HINT,
	PLMYSQL_GETDIAG_RETURNED_SQLSTATE,
	PLMYSQL_GETDIAG_COLUMN_NAME,
	PLMYSQL_GETDIAG_CONSTRAINT_NAME,
	PLMYSQL_GETDIAG_DATATYPE_NAME,
	PLMYSQL_GETDIAG_MESSAGE_TEXT,
	PLMYSQL_GETDIAG_TABLE_NAME,
	PLMYSQL_GETDIAG_SCHEMA_NAME
} PLmySQL_getdiag_kind;

/*
 * RAISE statement options
 */
typedef enum PLmySQL_raise_option_type
{
	PLMYSQL_RAISEOPTION_ERRCODE,
	PLMYSQL_RAISEOPTION_MESSAGE,
	PLMYSQL_RAISEOPTION_DETAIL,
	PLMYSQL_RAISEOPTION_HINT,
	PLMYSQL_RAISEOPTION_COLUMN,
	PLMYSQL_RAISEOPTION_CONSTRAINT,
	PLMYSQL_RAISEOPTION_DATATYPE,
	PLMYSQL_RAISEOPTION_TABLE,
	PLMYSQL_RAISEOPTION_SCHEMA
} PLmySQL_raise_option_type;

/*
 * Behavioral modes for plmysql variable resolution
 */
typedef enum PLmySQL_resolve_option
{
	PLMYSQL_RESOLVE_ERROR,		/* throw error if ambiguous */
	PLMYSQL_RESOLVE_VARIABLE,	/* prefer plmysql var to table column */
	PLMYSQL_RESOLVE_COLUMN		/* prefer table column to plmysql var */
} PLmySQL_resolve_option;


/**********************************************************************
 * Node and structure definitions
 **********************************************************************/

/*
 * mysql data type
 */
typedef struct PLmySQL_type
{
	char	   *typname;		/* (simple) name of the type */
	Oid			typoid;			/* OID of the data type */
	PLmySQL_type_type ttype;	/* PLMYSQL_TTYPE_ code */
	int16		typlen;			/* stuff copied from its pg_type entry */
	bool		typbyval;
	char		typtype;
	Oid			collation;		/* from my_type, but can be overridden */
	bool		typisarray;		/* is "true" array, or domain over one */
	int32		atttypmod;		/* typmod (taken from someplace else) */
	/* Remaining fields are used only for named composite types (not RECORD) */
	TypeName   *origtypname;	/* type name as written by user */
	TypeCacheEntry *tcache;		/* typcache entry for composite type */
	uint64		tupdesc_id;		/* last-seen tupdesc identifier */
} PLmySQL_type;

/*
 * SQL Query to plan and execute
 */
typedef struct PLmySQL_expr
{
	char	   *query;			/* query string, verbatim from function body */
	RawParseMode parseMode;		/* raw_parser() mode to use */
	SPIPlanPtr	plan;			/* plan, or NULL if not made yet */
	Bitmapset  *paramnos;		/* all dnos referenced by this query */

	/* function containing this expr (not set until we first parse query) */
	struct PLmySQL_function *func;

	/* namespace chain visible to this expr */
	struct PLmySQL_nsitem *ns;

	/* fields for "simple expression" fast-path execution: */
	Expr	   *expr_simple_expr;	/* NULL means not a simple expr */
	Oid			expr_simple_type;	/* result type Oid, if simple */
	int32		expr_simple_typmod; /* result typmod, if simple */
	bool		expr_simple_mutable;	/* true if simple expr is mutable */

	/*
	 * These fields are used to optimize assignments to expanded-datum
	 * variables.  If this expression is the source of an assignment to a
	 * simple variable, target_param holds that variable's dno; else it's -1.
	 * If we match a Param within expr_simple_expr to such a variable, that
	 * Param's address is stored in expr_rw_param; then expression code
	 * generation will allow the value for that Param to be passed read/write.
	 */
	int			target_param;	/* dno of assign target, or -1 if none */
	Param	   *expr_rw_param;	/* read/write Param within expr, if any */

	/*
	 * If the expression was ever determined to be simple, we remember its
	 * CachedPlanSource and CachedPlan here.  If expr_simple_plan_lxid matches
	 * current LXID, then we hold a refcount on expr_simple_plan in the
	 * current transaction.  Otherwise we need to get one before re-using it.
	 */
	CachedPlanSource *expr_simple_plansource;	/* extracted from "plan" */
	CachedPlan *expr_simple_plan;	/* extracted from "plan" */
	LocalTransactionId expr_simple_plan_lxid;

	/*
	 * if expr is simple AND prepared in current transaction,
	 * expr_simple_state and expr_simple_in_use are valid. Test validity by
	 * seeing if expr_simple_lxid matches current LXID.  (If not,
	 * expr_simple_state probably points at garbage!)
	 */
	ExprState  *expr_simple_state;	/* eval tree for expr_simple_expr */
	bool		expr_simple_in_use; /* true if eval tree is active */
	LocalTransactionId expr_simple_lxid;
} PLmySQL_expr;

/*
 * Generic datum array item
 *
 * PLmySQL_datum is the common supertype for PLmySQL_var, PLmySQL_row,
 * PLmySQL_rec, and PLmySQL_recfield.
 */
typedef struct PLmySQL_datum
{
	PLmySQL_datum_type dtype;
	int			dno;
} PLmySQL_datum;

/*
 * Scalar or composite variable
 *
 * The variants PLmySQL_var, PLmySQL_row, and PLmySQL_rec share these
 * fields.
 */
typedef struct PLmySQL_variable
{
	PLmySQL_datum_type dtype;
	int			dno;
	char	   *refname;
	int			lineno;
	bool		isconst;
	bool		notnull;
	PLmySQL_expr *default_val;
} PLmySQL_variable;

/*
 * Scalar variable
 *
 * DTYPE_VAR and DTYPE_PROMISE datums both use this struct type.
 * A PROMISE datum works exactly like a VAR datum for most purposes,
 * but if it is read without having previously been assigned to, then
 * a special "promised" value is computed and assigned to the datum
 * before the read is performed.  This technique avoids the overhead of
 * computing the variable's value in cases where we expect that many
 * functions will never read it.
 */
typedef struct PLmySQL_var
{
	PLmySQL_datum_type dtype;
	int			dno;
	char	   *refname;
	int			lineno;
	bool		isconst;
	bool		notnull;
	PLmySQL_expr *default_val;
	/* end of PLmySQL_variable fields */

	PLmySQL_type *datatype;

	/*
	 * Variables declared as CURSOR FOR <query> are mostly like ordinary
	 * scalar variables of type refcursor, but they have these additional
	 * properties:
	 */
	PLmySQL_expr *cursor_explicit_expr;
	int			cursor_explicit_argrow;
	int			cursor_options;

	/* Fields below here can change at runtime */

	Datum		value;
	bool		isnull;
	bool		freeval;

	/*
	 * The promise field records which "promised" value to assign if the
	 * promise must be honored.  If it's a normal variable, or the promise has
	 * been fulfilled, this is PLMYSQL_PROMISE_NONE.
	 */
	PLmySQL_promise_type promise;
} PLmySQL_var;

/*
 * Row variable - this represents one or more variables that are listed in an
 * INTO clause, FOR-loop targetlist, cursor argument list, etc.  We also use
 * a row to represent a function's OUT parameters when there's more than one.
 *
 * Note that there's no way to name the row as such from PL/pgSQL code,
 * so many functions don't need to support these.
 *
 * That also means that there's no real name for the row variable, so we
 * conventionally set refname to "(unnamed row)".  We could leave it NULL,
 * but it's too convenient to be able to assume that refname is valid in
 * all variants of PLmySQL_variable.
 *
 * isconst, notnull, and default_val are unsupported (and hence
 * always zero/null) for a row.  The member variables of a row should have
 * been checked to be writable at compile time, so isconst is correctly set
 * to false.  notnull and default_val aren't applicable.
 */
typedef struct PLmySQL_row
{
	PLmySQL_datum_type dtype;
	int			dno;
	char	   *refname;
	int			lineno;
	bool		isconst;
	bool		notnull;
	PLmySQL_expr *default_val;
	/* end of PLmySQL_variable fields */

	/*
	 * rowtupdesc is only set up if we might need to convert the row into a
	 * composite datum, which currently only happens for OUT parameters.
	 * Otherwise it is NULL.
	 */
	TupleDesc	rowtupdesc;

	int			nfields;
	char	  **fieldnames;
	int		   *varnos;
} PLmySQL_row;

/*
 * Record variable (any composite type, including RECORD)
 */
typedef struct PLmySQL_rec
{
	PLmySQL_datum_type dtype;
	int			dno;
	char	   *refname;
	int			lineno;
	bool		isconst;
	bool		notnull;
	PLmySQL_expr *default_val;
	/* end of PLmySQL_variable fields */

	/*
	 * Note: for non-RECORD cases, we may from time to time re-look-up the
	 * composite type, using datatype->origtypname.  That can result in
	 * changing rectypeid.
	 */

	PLmySQL_type *datatype;		/* can be NULL, if rectypeid is RECORDOID */
	Oid			rectypeid;		/* declared type of variable */
	/* RECFIELDs for this record are chained together for easy access */
	int			firstfield;		/* dno of first RECFIELD, or -1 if none */

	/* Fields below here can change at runtime */

	/* We always store record variables as "expanded" records */
	ExpandedRecordHeader *erh;
} PLmySQL_rec;

/*
 * Field in record
 */
typedef struct PLmySQL_recfield
{
	PLmySQL_datum_type dtype;
	int			dno;
	/* end of PLmySQL_datum fields */

	char	   *fieldname;		/* name of field */
	int			recparentno;	/* dno of parent record */
	int			nextfield;		/* dno of next child, or -1 if none */
	uint64		rectupledescid; /* record's tupledesc ID as of last lookup */
	ExpandedRecordFieldInfo finfo;	/* field's attnum and type info */
	/* if rectupledescid == INVALID_TUPLEDESC_IDENTIFIER, finfo isn't valid */
} PLmySQL_recfield;

/*
 * Item in the compilers namespace tree
 */
typedef struct PLmySQL_nsitem
{
	PLmySQL_nsitem_type itemtype;

	/*
	 * For labels, itemno is a value of enum PLmySQL_label_type. For other
	 * itemtypes, itemno is the associated PLmySQL_datum's dno.
	 */
	int			itemno;
	struct PLmySQL_nsitem *prev;
	char		name[FLEXIBLE_ARRAY_MEMBER];	/* nul-terminated string */
} PLmySQL_nsitem;

/*
 * Generic execution node
 */
typedef struct PLmySQL_stmt
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;

	/*
	 * Unique statement ID in this function (starting at 1; 0 is invalid/not
	 * set).  This can be used by a profiler as the index for an array of
	 * per-statement metrics.
	 */
	unsigned int stmtid;
} PLmySQL_stmt;

/*
 * One EXCEPTION condition name
 */
typedef struct PLmySQL_condition
{
	int			sqlerrstate;	/* SQLSTATE code */
	char	   *condname;		/* condition name (for debugging) */
	struct PLmySQL_condition *next;
} PLmySQL_condition;

/*
 * EXCEPTION block
 */
typedef struct PLmySQL_exception_block
{
	int			sqlstate_varno;
	int			sqlerrm_varno;
	List	   *exc_list;		/* List of WHEN clauses */
} PLmySQL_exception_block;

/*
 * One EXCEPTION ... WHEN clause
 */
typedef struct PLmySQL_exception
{
	int			lineno;
	PLmySQL_condition *conditions;
	List	   *action;			/* List of statements */
} PLmySQL_exception;

/*
 * Block of statements
 */
typedef struct PLmySQL_stmt_block
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;
	unsigned int stmtid;
	char	   *label;
	List	   *body;			/* List of statements */
	int			n_initvars;		/* Length of initvarnos[] */
	int		   *initvarnos;		/* dnos of variables declared in this block */
	PLmySQL_exception_block *exceptions;
} PLmySQL_stmt_block;

/*
 * Assign statement
 */
typedef struct PLmySQL_stmt_assign
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;
	unsigned int stmtid;
	int			varno;
	PLmySQL_expr *expr;
	char		 *varname;	 /* ReqID:SRS-MYSQL-SET */
} PLmySQL_stmt_assign;

/*
 * PERFORM statement
 */
typedef struct PLmySQL_stmt_perform
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;
	unsigned int stmtid;
	PLmySQL_expr *expr;
} PLmySQL_stmt_perform;

/*
 * CALL statement
 */
typedef struct PLmySQL_stmt_call
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;
	unsigned int stmtid;
	PLmySQL_expr *expr;
	bool		is_call;
	PLmySQL_variable *target;
} PLmySQL_stmt_call;

/*
 * COMMIT statement
 */
typedef struct PLmySQL_stmt_commit
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;
	unsigned int stmtid;
	bool		chain;
} PLmySQL_stmt_commit;

/*
 * ROLLBACK statement
 */
typedef struct PLmySQL_stmt_rollback
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;
	unsigned int stmtid;
	bool		chain;
} PLmySQL_stmt_rollback;

/*
 * GET DIAGNOSTICS item
 */
typedef struct PLmySQL_diag_item
{
	PLmySQL_getdiag_kind kind;	/* id for diagnostic value desired */
	int			target;			/* where to assign it */
} PLmySQL_diag_item;

/*
 * GET DIAGNOSTICS statement
 */
typedef struct PLmySQL_stmt_getdiag
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;
	unsigned int stmtid;
	bool		is_stacked;		/* STACKED or CURRENT diagnostics area? */
	List	   *diag_items;		/* List of PLmySQL_diag_item */
} PLmySQL_stmt_getdiag;

/*
 * IF statement
 */
typedef struct PLmySQL_stmt_if
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;
	unsigned int stmtid;
	PLmySQL_expr *cond;			/* boolean expression for THEN */
	List	   *then_body;		/* List of statements */
	List	   *elsif_list;		/* List of PLmySQL_if_elsif structs */
	List	   *else_body;		/* List of statements */
} PLmySQL_stmt_if;

/*
 * one ELSIF arm of IF statement
 */
typedef struct PLmySQL_if_elsif
{
	int			lineno;
	PLmySQL_expr *cond;			/* boolean expression for this case */
	List	   *stmts;			/* List of statements */
} PLmySQL_if_elsif;

/*
 * CASE statement
 */
typedef struct PLmySQL_stmt_case
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;
	unsigned int stmtid;
	PLmySQL_expr *t_expr;		/* test expression, or NULL if none */
	int			t_varno;		/* var to store test expression value into */
	List	   *case_when_list; /* List of PLmySQL_case_when structs */
	bool		have_else;		/* flag needed because list could be empty */
	List	   *else_stmts;		/* List of statements */
} PLmySQL_stmt_case;

/*
 * one arm of CASE statement
 */
typedef struct PLmySQL_case_when
{
	int			lineno;
	PLmySQL_expr *expr;			/* boolean expression for this case */
	List	   *stmts;			/* List of statements */
} PLmySQL_case_when;

/*
 * Unconditional LOOP statement
 */
typedef struct PLmySQL_stmt_loop
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;
	unsigned int stmtid;
	char	   *label;
	List	   *body;			/* List of statements */
} PLmySQL_stmt_loop;

/*
 * WHILE cond LOOP statement
 */
typedef struct PLmySQL_stmt_while
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;
	unsigned int stmtid;
	char	   *label;
	PLmySQL_expr *cond;
	List	   *body;			/* List of statements */
} PLmySQL_stmt_while;

/*
 * FOR statement with integer loopvar
 */
typedef struct PLmySQL_stmt_fori
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;
	unsigned int stmtid;
	char	   *label;
	PLmySQL_var *var;
	PLmySQL_expr *lower;
	PLmySQL_expr *upper;
	PLmySQL_expr *step;			/* NULL means default (ie, BY 1) */
	int			reverse;
	List	   *body;			/* List of statements */
} PLmySQL_stmt_fori;

/*
 * PLmySQL_stmt_forq represents a FOR statement running over a SQL query.
 * It is the common supertype of PLmySQL_stmt_fors, PLmySQL_stmt_forc
 * and PLmySQL_stmt_dynfors.
 */
typedef struct PLmySQL_stmt_forq
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;
	unsigned int stmtid;
	char	   *label;
	PLmySQL_variable *var;		/* Loop variable (record or row) */
	List	   *body;			/* List of statements */
} PLmySQL_stmt_forq;

/*
 * FOR statement running over SELECT
 */
typedef struct PLmySQL_stmt_fors
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;
	unsigned int stmtid;
	char	   *label;
	PLmySQL_variable *var;		/* Loop variable (record or row) */
	List	   *body;			/* List of statements */
	/* end of fields that must match PLmySQL_stmt_forq */
	PLmySQL_expr *query;
} PLmySQL_stmt_fors;

/*
 * FOR statement running over cursor
 */
typedef struct PLmySQL_stmt_forc
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;
	unsigned int stmtid;
	char	   *label;
	PLmySQL_variable *var;		/* Loop variable (record or row) */
	List	   *body;			/* List of statements */
	/* end of fields that must match PLmySQL_stmt_forq */
	int			curvar;
	PLmySQL_expr *argquery;		/* cursor arguments if any */
} PLmySQL_stmt_forc;

/*
 * FOR statement running over EXECUTE
 */
typedef struct PLmySQL_stmt_dynfors
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;
	unsigned int stmtid;
	char	   *label;
	PLmySQL_variable *var;		/* Loop variable (record or row) */
	List	   *body;			/* List of statements */
	/* end of fields that must match PLmySQL_stmt_forq */
	PLmySQL_expr *query;
	List	   *params;			/* USING expressions */
} PLmySQL_stmt_dynfors;

/*
 * FOREACH item in array loop
 */
typedef struct PLmySQL_stmt_foreach_a
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;
	unsigned int stmtid;
	char	   *label;
	int			varno;			/* loop target variable */
	int			slice;			/* slice dimension, or 0 */
	PLmySQL_expr *expr;			/* array expression */
	List	   *body;			/* List of statements */
} PLmySQL_stmt_foreach_a;

/*
 * OPEN a curvar
 */
typedef struct PLmySQL_stmt_open
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;
	unsigned int stmtid;
	int			curvar;
	int			cursor_options;
	PLmySQL_expr *argquery;
	PLmySQL_expr *query;
	PLmySQL_expr *dynquery;
	List	   *params;			/* USING expressions */
} PLmySQL_stmt_open;

/*
 * FETCH or MOVE statement
 */
typedef struct PLmySQL_stmt_fetch
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;
	unsigned int stmtid;
	PLmySQL_variable *target;	/* target (record or row) */
	int			curvar;			/* cursor variable to fetch from */
	FetchDirection direction;	/* fetch direction */
	long		how_many;		/* count, if constant (expr is NULL) */
	PLmySQL_expr *expr;			/* count, if expression */
	bool		is_move;		/* is this a fetch or move? */
	bool		returns_multiple_rows;	/* can return more than one row? */
} PLmySQL_stmt_fetch;

/*
 * CLOSE curvar
 */
typedef struct PLmySQL_stmt_close
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;
	unsigned int stmtid;
	int			curvar;
} PLmySQL_stmt_close;

/*
 * EXIT or CONTINUE statement
 */
typedef struct PLpgSQL_stmt_exit
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;
	unsigned int stmtid;
	bool		is_exit;		/* Is this an exit or a continue? */
	char	   *label;			/* NULL if it's an unlabeled EXIT/CONTINUE */
	PLmySQL_expr *cond;
} PLmySQL_stmt_exit;

/*
 * RETURN statement
 */
typedef struct PLmySQL_stmt_return
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;
	unsigned int stmtid;
	PLmySQL_expr *expr;
	int			retvarno;
} PLmySQL_stmt_return;

/*
 * RETURN NEXT statement
 */
typedef struct PLmySQL_stmt_return_next
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;
	unsigned int stmtid;
	PLmySQL_expr *expr;
	int			retvarno;
} PLmySQL_stmt_return_next;

/*
 * RETURN QUERY statement
 */
typedef struct PLmySQL_stmt_return_query
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;
	unsigned int stmtid;
	PLmySQL_expr *query;		/* if static query */
	PLmySQL_expr *dynquery;		/* if dynamic query (RETURN QUERY EXECUTE) */
	List	   *params;			/* USING arguments for dynamic query */
} PLmySQL_stmt_return_query;

/*
 * RAISE statement
 */
typedef struct PLmySQL_stmt_raise
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;
	unsigned int stmtid;
	int			elog_level;
	char	   *condname;		/* condition name, SQLSTATE, or NULL */
	char	   *message;		/* old-style message format literal, or NULL */
	List	   *params;			/* list of expressions for old-style message */
	List	   *options;		/* list of PLmySQL_raise_option */
} PLmySQL_stmt_raise;

/*
 * RAISE statement option
 */
typedef struct PLmySQL_raise_option
{
	PLmySQL_raise_option_type opt_type;
	PLmySQL_expr *expr;
} PLmySQL_raise_option;

/*
 * ASSERT statement
 */
typedef struct PLmySQL_stmt_assert
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;
	unsigned int stmtid;
	PLmySQL_expr *cond;
	PLmySQL_expr *message;
} PLmySQL_stmt_assert;

/*
 * Generic SQL statement to execute
 */
typedef struct PLmySQL_stmt_execsql
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;
	unsigned int stmtid;
	PLmySQL_expr *sqlstmt;
	bool		mod_stmt;		/* is the stmt INSERT/UPDATE/DELETE? */
	bool		into;			/* INTO supplied? */
	bool		strict;			/* INTO STRICT flag */
	bool		mod_stmt_set;	/* is mod_stmt valid yet? */
	PLmySQL_variable *target;	/* INTO target (record or row) */
} PLmySQL_stmt_execsql;

/*
 * Dynamic SQL string to execute
 */
typedef struct PLmySQL_stmt_dynexecute
{
	PLmySQL_stmt_type cmd_type;
	int			lineno;
	unsigned int stmtid;
	PLmySQL_expr *query;		/* string expression */
	bool		into;			/* INTO supplied? */
	bool		strict;			/* INTO STRICT flag */
	PLmySQL_variable *target;	/* INTO target (record or row) */
	List	   *params;			/* USING expressions */
} PLmySQL_stmt_dynexecute;

/*
 * Hash lookup key for functions
 */
typedef struct PLmySQL_func_hashkey
{
	Oid			funcOid;

	bool		isTrigger;		/* true if called as a DML trigger */
	bool		isEventTrigger; /* true if called as an event trigger */

	/* be careful that pad bytes in this struct get zeroed! */

	/*
	 * For a trigger function, the OID of the trigger is part of the hash key
	 * --- we want to compile the trigger function separately for each trigger
	 * it is used with, in case the rowtype or transition table names are
	 * different.  Zero if not called as a DML trigger.
	 */
	Oid			trigOid;

	/*
	 * We must include the input collation as part of the hash key too,
	 * because we have to generate different plans (with different Param
	 * collations) for different collation settings.
	 */
	Oid			inputCollation;

	/*
	 * We include actual argument types in the hash key to support polymorphic
	 * PLpgSQL functions.  Be careful that extra positions are zeroed!
	 */
	Oid			argtypes[FUNC_MAX_ARGS];
} PLmySQL_func_hashkey;

/*
 * Trigger type
 */
typedef enum PLmySQL_trigtype
{
	PLMYSQL_DML_TRIGGER,
	PLMYSQL_EVENT_TRIGGER,
	PLMYSQL_NOT_TRIGGER
} PLmySQL_trigtype;

/*
 * Complete compiled function
 */
typedef struct PLmySQL_function
{
	char	   *fn_signature;
	Oid			fn_oid;
	TransactionId fn_xmin;
	ItemPointerData fn_tid;
	PLmySQL_trigtype fn_is_trigger;
	Oid			fn_input_collation;
	PLmySQL_func_hashkey *fn_hashkey;	/* back-link to hashtable key */
	MemoryContext fn_cxt;

	Oid			fn_rettype;
	int			fn_rettyplen;
	bool		fn_retbyval;
	bool		fn_retistuple;
	bool		fn_retisdomain;
	bool		fn_retset;
	bool		fn_readonly;
	char		fn_prokind;

	int			fn_nargs;
	int			fn_argvarnos[FUNC_MAX_ARGS];
	int			out_param_varno;
	int			found_varno;
	int			new_varno;
	int			old_varno;

	PLmySQL_resolve_option resolve_option;

	bool		print_strict_params;

	/* extra checks */
	int			extra_warnings;
	int			extra_errors;

	/* the datums representing the function's local variables */
	int			ndatums;
	PLmySQL_datum **datums;
	Size		copiable_size;	/* space for locally instantiated datums */

	/* function body parsetree */
	PLmySQL_stmt_block *action;

	/* data derived while parsing body */
	unsigned int nstatements;	/* counter for assigning stmtids */
	bool		requires_procedure_resowner;	/* contains CALL or DO? */

	/* these fields change when the function is used */
	struct PLmySQL_execstate *cur_estate;
	unsigned long use_count;
} PLmySQL_function;

/*
 * Runtime execution data
 */
typedef struct PLmySQL_execstate
{
	PLmySQL_function *func;		/* function being executed */

	TriggerData *trigdata;		/* if regular trigger, data about firing */
	EventTriggerData *evtrigdata;	/* if event trigger, data about firing */

	Datum		retval;
	bool		retisnull;
	Oid			rettype;		/* type of current retval */

	Oid			fn_rettype;		/* info about declared function rettype */
	bool		retistuple;
	bool		retisset;

	bool		readonly_func;
	bool		atomic;

	char	   *exitlabel;		/* the "target" label of the current EXIT or
								 * CONTINUE stmt, if any */
	ErrorData  *cur_error;		/* current exception handler's error */

	Tuplestorestate *tuple_store;	/* SRFs accumulate results here */
	TupleDesc	tuple_store_desc;	/* descriptor for tuples in tuple_store */
	MemoryContext tuple_store_cxt;
	ResourceOwner tuple_store_owner;
	ReturnSetInfo *rsi;

	int			found_varno;

	/*
	 * The datums representing the function's local variables.  Some of these
	 * are local storage in this execstate, but some just point to the shared
	 * copy belonging to the PLmySQL_function, depending on whether or not we
	 * need any per-execution state for the datum's dtype.
	 */
	int			ndatums;
	PLmySQL_datum **datums;
	/* context containing variable values (same as func's SPI_proc context) */
	MemoryContext datum_context;

	/*
	 * paramLI is what we use to pass local variable values to the executor.
	 * It does not have a ParamExternData array; we just dynamically
	 * instantiate parameter data as needed.  By convention, PARAM_EXTERN
	 * Params have paramid equal to the dno of the referenced local variable.
	 */
	ParamListInfo paramLI;

	/* EState and resowner to use for "simple" expression evaluation */
	EState	   *simple_eval_estate;
	ResourceOwner simple_eval_resowner;

	/* if running nonatomic procedure or DO block, resowner to use for CALL */
	ResourceOwner procedure_resowner;

	/* lookup table to use for executing type casts */
	HTAB	   *cast_hash;
	MemoryContext cast_hash_context;

	/* memory context for statement-lifespan temporary values */
	MemoryContext stmt_mcontext;	/* current stmt context, or NULL if none */
	MemoryContext stmt_mcontext_parent; /* parent of current context */

	/* temporary state for results from evaluation of query or expr */
	SPITupleTable *eval_tuptable;
	uint64		eval_processed;
	ExprContext *eval_econtext; /* for executing simple expressions */

	/* status information for error context reporting */
	PLmySQL_stmt *err_stmt;		/* current stmt */
	const char *err_text;		/* additional state info */

	void	   *plugin_info;	/* reserved for use by optional plugin */
} PLmySQL_execstate;

/*
 * A PLmySQL_plugin structure represents an instrumentation plugin.
 * To instrument PL/mySQL, a plugin library must access the rendezvous
 * variable "PLmySQL_plugin" and set it to point to a PLmySQL_plugin struct.
 * Typically the struct could just be static data in the plugin library.
 * We expect that a plugin would do this at library load time (_PG_init()).
 * It must also be careful to set the rendezvous variable back to NULL
 * if it is unloaded (_PG_fini()).
 *
 * This structure is basically a collection of function pointers --- at
 * various interesting points in pl_exec.c, we call these functions
 * (if the pointers are non-NULL) to give the plugin a chance to watch
 * what we are doing.
 *
 * func_setup is called when we start a function, before we've initialized
 * the local variables defined by the function.
 *
 * func_beg is called when we start a function, after we've initialized
 * the local variables.
 *
 * func_end is called at the end of a function.
 *
 * stmt_beg and stmt_end are called before and after (respectively) each
 * statement.
 *
 * Also, immediately before any call to func_setup, PL/mySQL fills in the
 * error_callback and assign_expr fields with pointers to its own
 * plmysql_exec_error_callback and exec_assign_expr functions.  This is
 * a somewhat ad-hoc expedient to simplify life for debugger plugins.
 */
typedef struct PLmySQL_plugin
{
	/* Function pointers set up by the plugin */
	void		(*func_setup) (PLmySQL_execstate *estate, PLmySQL_function *func);
	void		(*func_beg) (PLmySQL_execstate *estate, PLmySQL_function *func);
	void		(*func_end) (PLmySQL_execstate *estate, PLmySQL_function *func);
	void		(*stmt_beg) (PLmySQL_execstate *estate, PLmySQL_stmt *stmt);
	void		(*stmt_end) (PLmySQL_execstate *estate, PLmySQL_stmt *stmt);

	/* Function pointers set by PL/pgSQL itself */
	void		(*error_callback) (void *arg);
	void		(*assign_expr) (PLmySQL_execstate *estate, PLmySQL_datum *target,
								PLmySQL_expr *expr);
} PLmySQL_plugin;

/*
 * Struct types used during parsing
 */

typedef struct PLword
{
	char	   *ident;			/* palloc'd converted identifier */
	bool		quoted;			/* Was it double-quoted? */
} PLword;

typedef struct PLcword
{
	List	   *idents;			/* composite identifiers (list of String) */
} PLcword;

typedef struct PLwdatum
{
	PLmySQL_datum *datum;		/* referenced variable */
	char	   *ident;			/* valid if simple name */
	bool		quoted;
	List	   *idents;			/* valid if composite name */
} PLwdatum;

/**********************************************************************
 * Global variable declarations
 **********************************************************************/

typedef enum
{
	IDENTIFIER_LOOKUP_NORMAL,	/* normal processing of var names */
	IDENTIFIER_LOOKUP_DECLARE,	/* In DECLARE --- don't look up names */
	IDENTIFIER_LOOKUP_EXPR		/* In SQL expression --- special case */
} IdentifierLookup;

extern IdentifierLookup plmysql_IdentifierLookup;

extern int	plmysql_variable_conflict;

extern bool plmysql_print_strict_params;

extern bool plmysql_check_asserts;

/* extra compile-time and run-time checks */
#define PLMYSQL_XCHECK_NONE						0
#define PLMYSQL_XCHECK_SHADOWVAR				(1 << 1)
#define PLMYSQL_XCHECK_TOOMANYROWS				(1 << 2)
#define PLMYSQL_XCHECK_STRICTMULTIASSIGNMENT	(1 << 3)
#define PLMYSQL_XCHECK_ALL						((int) ~0)

extern int	plmysql_extra_warnings;
extern int	plmysql_extra_errors;

extern bool plmysql_check_syntax;
extern bool plmysql_DumpExecTree;

extern PLmySQL_stmt_block *plmysql_parse_result;

extern int	plmysql_nDatums;
extern PLmySQL_datum **plmysql_Datums;

extern char *plmysql_error_funcname;

extern PLmySQL_function *plmysql_curr_compile;
extern MemoryContext plmysql_compile_tmp_cxt;

extern PLmySQL_plugin **plmysql_plugin_ptr;

/**********************************************************************
 * Function declarations
 **********************************************************************/

/*
 * Functions in pl_comp.c
 */
extern PLmySQL_function *plmysql_compile(FunctionCallInfo fcinfo,
										 bool forValidator);
extern PLmySQL_function *plmysql_compile_inline(char *proc_source);
extern void plmysql_parser_setup(struct ParseState *pstate,
								 PLmySQL_expr *expr);
extern bool plmysql_parse_word(char *word1, const char *yytxt, bool lookup,
							   PLwdatum *wdatum, PLword *word);
extern bool plmysql_parse_dblword(char *word1, char *word2,
								  PLwdatum *wdatum, PLcword *cword);
extern bool plmysql_parse_tripword(char *word1, char *word2, char *word3,
								   PLwdatum *wdatum, PLcword *cword);
extern PLmySQL_type *plmysql_parse_wordtype(char *ident);
extern PLmySQL_type *plmysql_parse_cwordtype(List *idents);
extern PLmySQL_type *plmysql_parse_wordrowtype(char *ident);
extern PLmySQL_type *plmysql_parse_cwordrowtype(List *idents);
extern PLmySQL_type *plmysql_build_datatype(Oid typeOid, int32 typmod,
											Oid collation,
											TypeName *origtypname);
extern PLmySQL_variable *plmysql_build_variable(const char *refname, int lineno,
												PLmySQL_type *dtype,
												bool add2namespace);
extern PLmySQL_rec *plmysql_build_record(const char *refname, int lineno,
										 PLmySQL_type *dtype, Oid rectypeid,
										 bool add2namespace);
extern PLmySQL_recfield *plmysql_build_recfield(PLmySQL_rec *rec,
												const char *fldname);
extern int	plmysql_recognize_err_condition(const char *condname,
											bool allow_sqlstate);
extern PLmySQL_condition *plmysql_parse_err_condition(char *condname);
extern void plmysql_adddatum(PLmySQL_datum *newdatum);
extern int	plmysql_add_initdatums(int **varnos);
extern void plmysql_HashTableInit(void);

/*
 * Functions in pl_handler.c
 */
extern void _PG_init(void);

/*
 * Functions in pl_exec.c
 */
extern Datum plmysql_exec_function(PLmySQL_function *func,
								   FunctionCallInfo fcinfo,
								   EState *simple_eval_estate,
								   ResourceOwner simple_eval_resowner,
								   ResourceOwner procedure_resowner,
								   bool atomic);
extern HeapTuple plmysql_exec_trigger(PLmySQL_function *func,
									  TriggerData *trigdata);
extern void plmysql_exec_event_trigger(PLmySQL_function *func,
									   EventTriggerData *trigdata);
extern void plmysql_xact_cb(XactEvent event, void *arg);
extern void plmysql_subxact_cb(SubXactEvent event, SubTransactionId mySubid,
							   SubTransactionId parentSubid, void *arg);
extern Oid	plmysql_exec_get_datum_type(PLmySQL_execstate *estate,
										PLmySQL_datum *datum);
extern void plmysql_exec_get_datum_type_info(PLmySQL_execstate *estate,
											 PLmySQL_datum *datum,
											 Oid *typeId, int32 *typMod,
											 Oid *collation);

/*
 * Functions for namespace handling in pl_funcs.c
 */
extern void plmysql_ns_init(void);
extern void plmysql_ns_push(const char *label,
							PLmySQL_label_type label_type);
extern void plmysql_ns_pop(void);
extern PLmySQL_nsitem *plmysql_ns_top(void);
extern void plmysql_ns_additem(PLmySQL_nsitem_type itemtype, int itemno, const char *name);
extern PLmySQL_nsitem *plmysql_ns_lookup(PLmySQL_nsitem *ns_cur, bool localmode,
										 const char *name1, const char *name2,
										 const char *name3, int *names_used);
extern PLmySQL_nsitem *plmysql_ns_lookup_label(PLmySQL_nsitem *ns_cur,
											   const char *name);
extern PLmySQL_nsitem *plmysql_ns_find_nearest_loop(PLmySQL_nsitem *ns_cur);

/*
 * Other functions in pl_funcs.c
 */
extern const char *plmysql_stmt_typename(PLmySQL_stmt *stmt);
extern const char *plmysql_getdiag_kindname(PLmySQL_getdiag_kind kind);
extern void plmysql_free_function_memory(PLmySQL_function *func);
extern void plmysql_dumptree(PLmySQL_function *func);

/*
 * Scanner functions in pl_scanner.c
 */
extern int	plmysql_base_yylex(void);
extern int	plmysql_yylex(void);
extern void plmysql_push_back_token(int token);
extern bool plmysql_token_is_unreserved_keyword(int token);
extern void plmysql_append_source_text(StringInfo buf,
									   int startlocation, int endlocation);
extern int	plmysql_peek(void);
extern void plmysql_peek2(int *tok1_p, int *tok2_p, int *tok1_loc,
						  int *tok2_loc);
extern int	plmysql_scanner_errposition(int location);
extern void plmysql_yyerror(const char *message) pg_attribute_noreturn();
extern int	plmysql_location_to_lineno(int location);
extern int	plmysql_latest_lineno(void);
extern void plmysql_scanner_init(const char *str);
extern void plmysql_scanner_finish(void);

/*
 * Externs in gram.y
 */
extern int	plmysql_yyparse(void);

#endif							/* PLPGSQL_H */
