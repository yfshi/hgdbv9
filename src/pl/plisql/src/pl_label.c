/*-------------------------------------------------------------------------
 *
 * pl_label.c		- Compiler label/GOTO of the PL/iSQL
 *			  procedural language
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * Copyright:
 * Copyright (c) 2009-2025, HighGo Software Co.,Ltd. All right reserved.
 *
 *
 * IDENTIFICATION
 *	  src/pl/plisql/src/pl_label.c
 *
 * The phased processing flow for labels and GOTO is as follows:
 * 1.Complation
 *   a.Collect labels and GOTO information.
 *   b.Check the legality of GOTO targets.
 * 2.Execution
 *   a.Jump to the target when executing GOTO statement.
 *
 * This file is for the complation phase.
 *
 * ReqID:SRS-PLSQL-GOTO
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "plisql.h"
#include "utils/memutils.h"


/*
 * Label item in the label scope tree.
 */
typedef struct PLiSQL_label
{
	char   *name;		/* label name */
	int		exptidx;	/* index in exception list */
	int		stmtidx;	/* index in block */
	int		blkpathid;	/* blockid of located path */
	int		lscopeid;	/* blockid of located scope */
	bool	conflicted;	/* redefined label? */
} PLiSQL_label;

/*
 * Scope of labels, same as namespace chain.
 */
typedef struct PLiSQL_label_scope
{
	char   *name;
	int		blockid;	/* from PLiSQL_label_compile_context->next_blockid */
	List   *labels;		/* all labels of current scope */
	struct PLiSQL_label_scope *prev;
} PLiSQL_label_scope;

/*
 * Logical container stack, record the statement path tree.
 */
typedef struct PLiSQL_block_path
{
	char   *name;
	int		blockid;		/* from PLiSQL_label_compile_context->next_blockid */
	int		nbranches;		/* index of next when in exception */
	int		nstatements;	/* index of next statement in block */
	bool	is_exception;	/* is exception path? */
	struct PLiSQL_block_path *parent;
} PLiSQL_block_path;

/*
 * A GOTO statement item
 */
typedef struct PLiSQL_goto
{
	PLiSQL_stmt_goto	*stmt;		/* GOTO statement node */
	PLiSQL_label_scope	*lscope;	/* label scope stack */
	PLiSQL_block_path 	*blkpath;	/* block path stack */
} PLiSQL_goto;

/*
 * All label and GOTO data entry.
 */
typedef struct PLiSQL_label_compile_context
{
	PLiSQL_label_scope	*lscope_top;	/* top of label scope stack */
	PLiSQL_block_path	*blkpath_top;	/* top of block path stack */
	List				*goto_stmts;	/* GOTO statment list */
	int	next_blockid;					/* identification label scope and path */
} PLiSQL_label_compile_context;


/* Label collector for compiled functions */
static PLiSQL_label_compile_context *compile_context;


/*
 * plisql_lscope_lookup
 *   Lookup an label in the given scope chain.
 */
static PLiSQL_label *
plisql_lscope_lookup(PLiSQL_label_scope *scope, const char *name)
{
	while (scope != NULL)
	{
		ListCell *lc;

		foreach(lc, scope->labels)
		{
			PLiSQL_label *label = (PLiSQL_label *) lfirst(lc);

			if (strcmp(label->name, name) == 0)
				return label;
		}

		scope = scope->prev;
	}

	return NULL;
}

/*
 * plisql_blkpath_lookup
 *   Lookup an label in the given path stack and return the
 * number of nodes passed through.
 */
static int
plisql_blkpath_lookup(PLiSQL_block_path *path, const int blockid)
{
	int n = 0;

	while (path != NULL)
	{
		if (path->blockid == blockid)
			return n;

		path = path->parent;
		n++;
	}

	return -1;
}

/*
 * plisql_check_goto
 *   Check the legality of all GOTO target after compilation.
 */
static void
plisql_check_goto(void)
{
	ListCell *lc;

	/*
	 * Iterate each GOTO statement.
	 */
	foreach(lc, compile_context->goto_stmts)
	{
		int layers;
		PLiSQL_label *label;
		PLiSQL_goto *item = (PLiSQL_goto *) lfirst(lc);
		PLiSQL_label_scope *lscope = item->lscope;
		PLiSQL_stmt_goto *stmt = item->stmt;

		/*
		 * Lookup an label in the scope chain.
		 */
		label = plisql_lscope_lookup(lscope, stmt->target_label);
		if (label == NULL)
		{
			ereport(ERROR,
					(errmsg("invalid GOTO to non-label '%s'", stmt->target_label)));
			return;
		}

		/* Multiple label are declared in one scope. */
		if (label->conflicted)
		{
			ereport(ERROR,
					(errmsg("at most one declaration for '%s' is permitted", label->name)));
			return;
		}

		/*
		 * Lookup an label in the path stack by blockid and return the number
		 * of nodes passed through.
		 */
		layers = plisql_blkpath_lookup(item->blkpath, label->blkpathid);
		if (layers < 0)
		{
			/*
			 * The target label was found in the scope but not in the path.
			 */
			ereport(ERROR,
					(errmsg("illegal GOTO statement; this GOTO cannot branch to label '%s'", stmt->target_label)));
			return;
		}

		/*
		 * Fill in the data required for executing the GOTO statement, as follows:
		 * - target_layers: the number of iterations from GOTO to the target.
		 * - target_exptidx: the exception branch number where the target is located.
		 * - target_stmtidx: the index number of the target in its block.
		 */
		stmt->target_layers = layers;
		stmt->target_exptidx = label->exptidx;
		stmt->target_stmtidx = label->stmtidx;
	}
}

/*
 * plisql_blkpath_push_ext
 *   Create a new path node with the specified name and blockid;
 */
static void
plisql_blkpath_push_ext(const char *name, const int blockid)
{
	MemoryContext oldCxt;
	PLiSQL_block_path *new;

	/* Avoid memory leaks in the long-term function context */
	oldCxt = MemoryContextSwitchTo(plisql_compile_tmp_cxt);

	new = palloc0(sizeof(PLiSQL_block_path));
	new->name = pstrdup(name);
	new->blockid = blockid;
	new->nstatements = 0;
	new->is_exception = false;
	new->nbranches = 0;
	new->parent = compile_context->blkpath_top;
	compile_context->blkpath_top = new;

	MemoryContextSwitchTo(oldCxt);
}

/*
 * plisql_blkpath_push_ext
 *   Create a new scope node with the specified name and blockid;
 */
static void
plisql_lscope_push_ext(const char *name, const int blockid)
{
	MemoryContext oldCxt;
	PLiSQL_label_scope *new;

	/* Avoid memory leaks in the long-term function context */
	oldCxt = MemoryContextSwitchTo(plisql_compile_tmp_cxt);

	new = palloc0(sizeof(PLiSQL_label_scope));
	new->name = pstrdup(name);
	new->blockid = blockid;
	new->labels = NIL;
	new->prev = compile_context->lscope_top;
	compile_context->lscope_top = new;

	MemoryContextSwitchTo(oldCxt);
}

/*
 * plisql_start_label_compile
 *   Initialize label collector in complilation startup.
 */
void
plisql_start_label_compile(const char *funcname)
{
	PLiSQL_label_compile_context *context =
		MemoryContextAllocZero(plisql_compile_tmp_cxt,
							   sizeof(PLiSQL_label_compile_context));

	context->lscope_top = NULL;
	context->blkpath_top = NULL;
	context->goto_stmts = NIL;
	context->next_blockid = 0;

	compile_context = context;

	plisql_lscope_push(funcname);
}

/*
 * plisql_finish_label_compile
 *   Complete label collection and check the legality of GOTO.
 */
void
plisql_finish_label_compile(void)
{
	plisql_lscope_pop();
	plisql_check_goto();
}

/*
 * plisql_label_compile_context
 *   Return the global compile context.
 */
void *
plisql_label_compile_context(void)
{
	return compile_context;
}

/*
 * plisql_set_label_compile_context
 *   Set the global compile context.
 */
void
plisql_set_label_compile_context(void *context)
{
	compile_context = context;
}

/*
 * plisql_blkpath_push
 *   Create a new path node.
 */
void
plisql_blkpath_push(const char *name)
{
	plisql_blkpath_push_ext(name, compile_context->next_blockid++);
}

/*
 * plisql_blkpath_pop
 *   Pop the top path.
 */
void
plisql_blkpath_pop(void)
{
	compile_context->blkpath_top = compile_context->blkpath_top->parent;
}

/*
 * plisql_blkpath_update_nstatements
 *   Set the next statement index for the top path.
 */
void
plisql_blkpath_update_nstatements(bool reset)
{
	if (reset)
		compile_context->blkpath_top->nstatements = 0;
	else
		compile_context->blkpath_top->nstatements++;
}

/*
 * plisql_blkpath_set_exception
 *   Set the exception flag for the current block path.
 */
void
plisql_blkpath_set_exception(bool is_exception)
{
	compile_context->blkpath_top->is_exception = is_exception;
}

/*
 * plisql_blkpath_set_exptidx
 *   Set the next statement index for the top path.
 */
void
plisql_blkpath_update_nbranches(bool reset)
{
	Assert(compile_context->blkpath_top->is_exception);
	if (reset)
		compile_context->blkpath_top->nbranches = 0;
	else
		compile_context->blkpath_top->nbranches++;
}

/*
 * plisql_lscope_push
 *   Create a new scope node.
 */
void
plisql_lscope_push(const char *name)
{
	int blockid = compile_context->next_blockid++;

	if (name == NULL)
		name = "";
	plisql_lscope_push_ext(name, blockid);
	plisql_blkpath_push_ext(name, blockid);
}

/*
 * plisql_lscope_pop
 *   Pop the top scope node.
 */
void
plisql_lscope_pop(void)
{
	compile_context->lscope_top = compile_context->lscope_top->prev;
	plisql_blkpath_pop();
}

/*
 * plisql_register_label
 *   Add an label item to the current scope.
 */
void
plisql_register_label(const char *name)
{
	ListCell *lc;
	MemoryContext oldCxt;
	PLiSQL_label *label = NULL;
	PLiSQL_label_scope *lscope_top = compile_context->lscope_top;
	PLiSQL_block_path  *blkpath_top = compile_context->blkpath_top;

	/*
	 * Set the statement index number, and if currently in the exception
	 * section, set the branch index of the exception as needed.
	 *
	 * eg.
	 * begin
	 *   perform 1/0;                 -- block path {id:0, is_exception:false, stmtidx:0}
	 *   NULL;                        -- block path {id:0, is_exception:false, stmtidx:0}
	 * exception
	 *   when division_by_zero then   -- block path {id:0, is_exception:true, exptidx:0}
	 *     NULL;                      -- block path {id:0, is_exception:true, exptidx:0, stmtidx:0}
	 *     NULL;                      -- block path {id:0, is_exception:true, exptidx:0, stmtidx:1}
	 *   when others then             -- block path {id:0, is_exception:true, exptidx:1}
	 *     NULL;                      -- block path {id:0, is_exception:true, exptidx:1, stmtidx:0}
	 *     NULL;                      -- block path {id:0, is_exception:true, exptidx:1, stmtidx:1}
	 * end;
	 */

	/* Do nothing if label name is NULL */
	if (name == NULL)
		return;

	foreach(lc, lscope_top->labels)
	{
		PLiSQL_label *tmplabel = (PLiSQL_label *) lfirst(lc);

		if (strcmp(tmplabel->name, name) == 0 && tmplabel->lscopeid == lscope_top->blockid)
		{
			/* Found label with the same name in the current scope */
			tmplabel->conflicted = true;
			return;
		}
	}

	/* Avoid memory leaks in long-term function context */
	oldCxt = MemoryContextSwitchTo(plisql_compile_tmp_cxt);

	label = palloc0(sizeof(PLiSQL_label));
	label->name = pstrdup(name);
	label->exptidx = blkpath_top->is_exception ? blkpath_top->nbranches : -1;
	label->stmtidx = blkpath_top->nstatements;
	label->blkpathid = blkpath_top->blockid;
	label->lscopeid = lscope_top->blockid;
	label->conflicted = false;

	lscope_top->labels = lcons(label, lscope_top->labels);

	MemoryContextSwitchTo(oldCxt);
}

/*
 * plisql_register_goto
 *   Append an GOTO statement to the compile context.
 */
void
plisql_register_goto(PLiSQL_stmt_goto *stmt)
{
	MemoryContext oldCxt;
	PLiSQL_goto *new;

	/* Avoid memory leaks in long-term function context */
	oldCxt = MemoryContextSwitchTo(plisql_compile_tmp_cxt);

	new = palloc0(sizeof(PLiSQL_goto));
	new->stmt = stmt;
	new->lscope = compile_context->lscope_top;
	new->blkpath = compile_context->blkpath_top;

	compile_context->goto_stmts = lappend(compile_context->goto_stmts, new);

	MemoryContextSwitchTo(oldCxt);
}
