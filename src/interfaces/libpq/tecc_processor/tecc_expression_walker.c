/*-------------------------------------------------------------------------
 *
 * tecc_expression_walker.c
 *    Unified expression tree walker for TECC frontend processors.
 *
 * This module centralizes the knowledge of expression tree structure —
 * which child fields each node type has — so that individual processors
 * (stmt_processor, where_clause_processor, expr_processor) don't need
 * to duplicate the tree-traversal switch statements.
 *
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/interfaces/libpq/tecc_processor/tecc_expression_walker.c
 *
 *-------------------------------------------------------------------------
 */

/* ReqID:TECC4.0 */

#include "postgres_fe.h"
#include "tecc_expression_walker.h"
#include "internal/tecc_processor_internal.h"
#include "../frontend_parser/tecc_nodes.h"

/*
 * Forward declaration: walk a single node, used for list elements.
 */
static bool walk_node(Node *node, const TeccExprWalkerCallbacks *callbacks,
                      void *context);

/*
 * Walk a list of expression nodes.
 */
static bool
walk_list(List *list, const TeccExprWalkerCallbacks *callbacks, void *context)
{
    ListCell *lc;

    if (list == NIL)
        return true;

    foreach(lc, list)
    {
        Node *item = (Node *) lfirst(lc);
        if (item != NULL)
        {
            if (!walk_node(item, callbacks, context))
                return false;
        }
    }
    return true;
}

/*
 * Walk a list of SortBy nodes (used in ORDER BY, agg_order, window ORDER BY).
 */
static bool
walk_sortby_list(List *list, const TeccExprWalkerCallbacks *callbacks,
                 void *context)
{
    ListCell *lc;

    if (list == NIL)
        return true;

    foreach(lc, list)
    {
        SortBy *sb = (SortBy *) lfirst(lc);
        if (sb != NULL && sb->node != NULL)
        {
            if (!walk_node(sb->node, callbacks, context))
                return false;
        }
    }
    return true;
}

/*
 * Walk a target list (list of ResTarget nodes).
 */
static bool
walk_target_list(List *list, const TeccExprWalkerCallbacks *callbacks,
                 void *context)
{
    ListCell *lc;

    if (list == NIL)
        return true;

    foreach(lc, list)
    {
        ResTarget *rt = (ResTarget *) lfirst(lc);
        if (rt != NULL && rt->val != NULL)
        {
            if (!walk_node(rt->val, callbacks, context))
                return false;
        }
    }
    return true;
}

/*
 * Walk a subselect (SelectStmt or SetOperationStmt) that appears inside
 * a SubLink.  Walks expression-bearing fields: WHERE, HAVING, target list,
 * GROUP BY.
 */
static bool
walk_subselect(Node *subselect, const TeccExprWalkerCallbacks *callbacks,
               void *context)
{
    if (subselect == NULL)
        return true;

    switch (nodeTag(subselect))
    {
        case FE_T_SelectStmt:
        {
            SelectStmt *ss = (SelectStmt *) subselect;

            if (ss->whereClause != NULL)
            {
                if (!walk_node(ss->whereClause, callbacks, context))
                    return false;
            }
            if (ss->havingClause != NULL)
            {
                if (!walk_node(ss->havingClause, callbacks, context))
                    return false;
            }
            if (ss->targetList != NIL)
            {
                if (!walk_target_list(ss->targetList, callbacks, context))
                    return false;
            }
            if (ss->groupClause != NIL)
            {
                if (!walk_list(ss->groupClause, callbacks, context))
                    return false;
            }
            return true;
        }
        case FE_T_SetOp:
        {
            SetOperationStmt *sop = (SetOperationStmt *) subselect;

            if (sop->larg != NULL)
            {
                if (!walk_subselect(sop->larg, callbacks, context))
                    return false;
            }
            if (sop->rarg != NULL)
            {
                if (!walk_subselect(sop->rarg, callbacks, context))
                    return false;
            }
            return true;
        }
        default:
            /* Unknown subquery type — nothing to walk */
            return true;
    }
}

/*
 * Core walker: walk a single node.
 *
 * Calls pre_visit, recurses into children, calls post_visit.
 * Returns false if any callback returned false.
 */
static bool
walk_node(Node *node, const TeccExprWalkerCallbacks *callbacks, void *context)
{
    TeccExprVisitFunc pre_visit;
    TeccExprVisitFunc post_visit;

    if (node == NULL)
        return true;

    pre_visit  = callbacks ? callbacks->pre_visit : NULL;
    post_visit = callbacks ? callbacks->post_visit : NULL;

    /* Pre-visit */
    if (pre_visit != NULL)
    {
        if (!pre_visit(node, context))
            return false;
    }

    /* Recurse into children based on node type */
    switch (nodeTag(node))
    {
        /* ---- Leaf nodes (no children) ---- */
        case FE_T_ColumnRef:
        case FE_T_Const:
        case FE_T_Param:
        case FE_T_SetToDefault:
        case FE_T_CommonTableExpr:
        case FE_T_ResTarget:
            break;

        /* ---- Single-child nodes ---- */
        case FE_T_NullTest:
        {
            NullTest *nt = (NullTest *) node;
            if (nt->arg != NULL)
            {
                if (!walk_node(nt->arg, callbacks, context))
                    return false;
            }
            break;
        }

        case FE_T_BooleanTest:
        {
            struct BooleanTest *bt = (struct BooleanTest *) node;
            if (bt->arg != NULL)
            {
                if (!walk_node(bt->arg, callbacks, context))
                    return false;
            }
            break;
        }

        case FE_T_TypeCast:
        {
            TypeCast *tc = (TypeCast *) node;
            if (tc->arg != NULL)
            {
                if (!walk_node(tc->arg, callbacks, context))
                    return false;
            }
            break;
        }

        case FE_T_CollateClause:
        {
            CollateClause *cc = (CollateClause *) node;
            if (cc->arg != NULL)
            {
                if (!walk_node(cc->arg, callbacks, context))
                    return false;
            }
            break;
        }

        case FE_T_A_Indirection:
        {
            struct A_Indirection *ai = (struct A_Indirection *) node;
            if (ai->arg != NULL)
            {
                if (!walk_node(ai->arg, callbacks, context))
                    return false;
            }
            break;
        }

        /* ---- TimeZoneExpr: arg + timezone ---- */
        case FE_T_TimeZoneExpr:
        {
            struct TimeZoneExpr *tz = (struct TimeZoneExpr *) node;
            if (tz->arg != NULL)
            {
                if (!walk_node(tz->arg, callbacks, context))
                    return false;
            }
            if (tz->timezone != NULL)
            {
                if (!walk_node(tz->timezone, callbacks, context))
                    return false;
            }
            break;
        }

        /* ---- A_Expr: lexpr + rexpr + escape ---- */
        case FE_T_A_Expr:
        {
            A_Expr *ae = (A_Expr *) node;
            if (ae->lexpr != NULL)
            {
                if (!walk_node(ae->lexpr, callbacks, context))
                    return false;
            }
            if (ae->rexpr != NULL)
            {
                if (!walk_node(ae->rexpr, callbacks, context))
                    return false;
            }
            if (ae->escape != NULL)
            {
                if (!walk_node(ae->escape, callbacks, context))
                    return false;
            }
            break;
        }

        /* ---- BoolExpr: args list ---- */
        case FE_T_BoolExpr:
        {
            BoolExpr *be = (BoolExpr *) node;
            if (be->args != NIL)
            {
                if (!walk_list(be->args, callbacks, context))
                    return false;
            }
            break;
        }

        /* ---- RowExpr: args list ---- */
        case FE_T_RowExpr:
        {
            struct RowExpr *rx = (struct RowExpr *) node;
            if (rx->args != NIL)
            {
                if (!walk_list(rx->args, callbacks, context))
                    return false;
            }
            break;
        }

        /* ---- ArrayExpr: elements list ---- */
        case FE_T_ArrayExpr:
        {
            struct ArrayExpr *ae = (struct ArrayExpr *) node;
            if (ae->elements != NIL)
            {
                if (!walk_list(ae->elements, callbacks, context))
                    return false;
            }
            break;
        }

        /* ---- FuncCall: args + agg_filter + agg_order + over ---- */
        case FE_T_FuncCall:
        {
            FuncCall *fc = (FuncCall *) node;
            if (fc->args != NIL)
            {
                if (!walk_list(fc->args, callbacks, context))
                    return false;
            }
            if (fc->agg_filter != NULL)
            {
                if (!walk_node(fc->agg_filter, callbacks, context))
                    return false;
            }
            if (fc->agg_order != NIL)
            {
                if (!walk_sortby_list(fc->agg_order, callbacks, context))
                    return false;
            }
            if (fc->over != NULL)
            {
                if (!walk_node((Node *) fc->over, callbacks, context))
                    return false;
            }
            break;
        }

        /* ---- CaseExpr: arg + CaseWhen list + defresult ---- */
        case FE_T_CaseExpr:
        {
            struct CaseExpr *ce = (struct CaseExpr *) node;
            ListCell *lc;

            if (ce->arg != NULL)
            {
                if (!walk_node(ce->arg, callbacks, context))
                    return false;
            }
            if (ce->args != NIL)
            {
                foreach(lc, ce->args)
                {
                    struct CaseWhen *cw = (struct CaseWhen *) lfirst(lc);
                    if (cw != NULL)
                    {
                        if (cw->expr != NULL)
                        {
                            if (!walk_node(cw->expr, callbacks, context))
                                return false;
                        }
                        if (cw->result != NULL)
                        {
                            if (!walk_node(cw->result, callbacks, context))
                                return false;
                        }
                    }
                }
            }
            if (ce->defresult != NULL)
            {
                if (!walk_node(ce->defresult, callbacks, context))
                    return false;
            }
            break;
        }

        /* ---- SubLink: testexpr + subselect ---- */
        case FE_T_SubLink:
        {
            struct SubLink *sl = (struct SubLink *) node;

            if (sl->testexpr != NULL)
            {
                if (!walk_node(sl->testexpr, callbacks, context))
                    return false;
            }
            if (sl->subselect != NULL)
            {
                if (!walk_subselect(sl->subselect, callbacks, context))
                    return false;
            }
            break;
        }

        /* ---- SetOp: larg + rarg (standalone, not inside SubLink) ---- */
        case FE_T_SetOp:
        {
            SetOperationStmt *sop = (SetOperationStmt *) node;
            if (sop->larg != NULL)
            {
                if (!walk_node(sop->larg, callbacks, context))
                    return false;
            }
            if (sop->rarg != NULL)
            {
                if (!walk_node(sop->rarg, callbacks, context))
                    return false;
            }
            break;
        }

        /* ---- WindowDef: partitionClause + orderClause ---- */
        case FE_T_WindowDef:
        {
            struct WindowDef *wd = (struct WindowDef *) node;

            if (wd->partitionClause != NIL)
            {
                if (!walk_list(wd->partitionClause, callbacks, context))
                    return false;
            }
            if (wd->orderClause != NIL)
            {
                if (!walk_sortby_list(wd->orderClause, callbacks, context))
                    return false;
            }
            break;
        }

        default:
            /* Unknown node type — callbacks fired but no children walked */
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "tecc_walk_expression_tree: unhandled node type %d",
                               nodeTag(node));
            break;
    }

    /* Post-visit */
    if (post_visit != NULL)
    {
        if (!post_visit(node, context))
            return false;
    }

    return true;
}

/*
 * Public entry point: walk an expression tree.
 *
 * For each node, calls pre_visit (if non-NULL), recurses into children,
 * then calls post_visit (if non-NULL).  Returns false if any callback
 * returned false.
 */
bool
tecc_walk_expression_tree(Node *node,
                          const TeccExprWalkerCallbacks *callbacks,
                          void *context)
{
    if (node == NULL)
        return true;

    return walk_node(node, callbacks, context);
}

/* End - ReqID:TECC4.0 */
