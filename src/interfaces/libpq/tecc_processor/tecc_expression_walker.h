/*-------------------------------------------------------------------------
 *
 * tecc_expression_walker.h
 *    Unified expression tree walker for TECC frontend processors.
 *
 * Replaces three independent expression tree traversal implementations
 * (stmt_processor, where_clause_processor, expr_processor) with a single
 * walker that knows the tree structure.  Each processor provides callbacks
 * for node-specific actions.
 *
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/interfaces/libpq/tecc_processor/tecc_expression_walker.h
 *
 *-------------------------------------------------------------------------
 */

#ifndef TECC_EXPRESSION_WALKER_H
#define TECC_EXPRESSION_WALKER_H

#include "postgres_fe.h"
#include "../frontend_parser/tecc_nodes.h"

/* ReqID:TECC4.0 */

/*
 * Callback function type for expression tree walking.
 *
 * Called for each node during traversal.  Return false to stop traversal
 * (error condition), true to continue.
 *
 * When used as pre_visit: called before recursing into children.
 * When used as post_visit: called after recursing into children.
 */
typedef bool (*TeccExprVisitFunc)(Node *node, void *context);

/*
 * Walker callback structure.
 * Both callbacks are optional (NULL means no-op for that event).
 */
typedef struct TeccExprWalkerCallbacks
{
    TeccExprVisitFunc pre_visit;
    TeccExprVisitFunc post_visit;
} TeccExprWalkerCallbacks;

/*
 * Walk an expression tree.
 *
 * For each node:
 *   1. Call callbacks->pre_visit(node, context) if non-NULL.
 *      If it returns false, stop and return false.
 *   2. Recurse into child nodes (fields, lists, subqueries).
 *   3. Call callbacks->post_visit(node, context) if non-NULL.
 *      If it returns false, stop and return false.
 *
 * Returns false if any callback returned false, true otherwise.
 */
bool tecc_walk_expression_tree(Node *node,
                               const TeccExprWalkerCallbacks *callbacks,
                               void *context);

#endif /* TECC_EXPRESSION_WALKER_H */
