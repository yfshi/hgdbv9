/* Begin - ReqID:TECC4.0 */
/*
 * File: where_clause_processor.c
 *
 * Abstract:
 *   WHERE clause processor implementation for TECC processor module.
 *   Implements processing of WHERE clauses including operator
 *   compatibility checking and encryption of comparison values.
 *
 * Authored by <wangxp@highgo.com>, 20260403.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/where_clause_processor.c
 */

#include "postgres_fe.h"
#include "libpq-fe.h"              /* For PQdb() function */
#include "common/shortest_dec.h"   /* ReqID:TECC4.0 - float/double_to_shortest_decimal_buf */
#include "where_clause_processor.h"
#include "stmt_processor.h"
#include "values_processor.h"
#include "raw_value.h"  /* For TeccRawValue functions */
#include "internal/tecc_processor_internal.h"  /* For error handling and debug functions */

/* Include PostgreSQL type definitions for OID constants */
#include "catalog/pg_type_d.h"  /* For type OID constants (INT2OID, INT4OID, etc.) */

/* Include TECC type definitions for encrypted type OIDs */
#include "../../../src/include/utils/tecc/tecc_type_mapping.h"  /* For encrypted type OIDs */

/* Include frontend parser node definitions */
#include "../frontend_parser/fe_adapter.h"  /* Frontend node definitions */
#include "../frontend_parser/tecc_nodes.h"  /* For foreach macro and list functions */
/* Note: Frontend uses Const structure, not A_Const */

/* Include TeccProcessorColumnRef for encrypted_columns list access */
#include "internal/tecc_processor_internal.h"

/* Forward declarations for external module interfaces */
#include "../tecc_cache/tecc_metadata_cache.h"  /* Cache module interface */
#include "../tecc_scheduler/tecc_crypto_scheduler.h"  /* Crypto scheduler interface */
#include "../tecc_formatter/tecc_formatter.h"  /* Data formatter interface */
#include "../tecc_libpq_integration/fe-tecc-integration.h"  /* For tecc_get_state() */
#include "expr_processor.h"  /* Expression processor interface */
#include "column_ref_data.h"  /* Column reference data functions */
#include "tecc_expression_walker.h"  /* ReqID:TECC4.0 - Unified expression walker */

/*
 * Forward declarations
 */
static TeccWhereClauseProcessorResult where_process_expr(Node *expr,
                                                        TeccStatementData *statement_data);
static TeccWhereClauseProcessorResult where_process_a_expr(struct A_Expr *expr,
                                                          TeccStatementData *statement_data);
static TeccWhereClauseProcessorResult where_process_subquery_target_consts(
    Node *subquery, Node *testexpr, const char *operator_name,
    TeccStatementData *statement_data);
/* Helper function declarations */
static const char *where_get_column_kek_name(TeccStatementData *statement_data,
                                             const char *schema_name,
                                             const char *table_name,
                                             const char *column_name);
static TeccWhereClauseProcessorResult where_process_column_ref(struct ColumnRef *col_ref,
                                                              TeccStatementData *statement_data);
static bool where_is_encrypted_column(const char *column_name,
                                     TeccStatementData *statement_data);
static bool where_is_encrypted_column_full(const char *table_name,
                                          const char *column_name,
                                          TeccStatementData *statement_data);
static const TeccOperatorSupport *where_get_operator_support(const char *operator_name);
static char *where_extract_const_value(Const *const_expr, TeccStatementData *statement_data);
/* ReqID:TECC4.0 - Resolve DEK name of the encrypted column referenced by expr */
static char *where_get_column_dek_from_expr(Node *expr, TeccStatementData *statement_data);
/* ReqID:TECC4.0 - Reject IN/ANY/ALL subquery comparisons across different DEKs */
static TeccWhereClauseProcessorResult where_check_sublink_dek(struct SubLink *sl,
                                                             Node *outer_expr,
                                                             TeccStatementData *statement_data);
/* ReqID:TECC4.0 - Load subquery FROM-table encrypted columns for DEK resolution */
static void where_load_sublink_tables(struct SubLink *sl, TeccStatementData *statement_data);
/* ReqID:TECC4.0 - Extract constant info from Const or A_Expr(UMINUS,Const) */
static bool where_extract_const_info(Node *node, const char *query,
                                     size_t *loc, size_t *dsize,
                                     char **text, Oid *ctype);
static TeccWhereClauseProcessorResult where_rewrite_expr(Node *expr, TeccStatementData *statement_data);
static TeccWhereClauseProcessorResult where_rewrite_a_expr(struct A_Expr *expr, TeccStatementData *statement_data);
static TeccWhereClauseProcessorResult where_rewrite_comparison(Node *lexpr, Node *rexpr,
                                                              const char *operator_name,
                                                              TeccStatementData *statement_data);
static int where_replace_const_value(Node **expr_ptr, const char *encrypted_value,
                                    TeccStatementData *statement_data,
                                    Oid encrypted_type_oid);
static Oid where_get_encrypted_type_oid(Oid base_type_oid);
static TeccWhereClauseProcessorResult where_check_algorithm_for_operator(
    const char *operator_name,
    TeccColumnCacheItem *column_item,
    TeccStatementData *statement_data);

/*
 * Extract WHERE clause from parse tree
 */
static Node *where_extract_clause_from_parse_tree(List *parse_tree);

/*
 * Operator support table
 */
static const TeccOperatorSupport operator_support_table[] = {
    /* Comparison operators
     * Note: OPE保序加密也支持等值比较，所以=和<>的supports_ope=true */
    {"=", true, true, false, false, "DET"},
    {"<>", true, true, false, false, "DET"},
    {"!=", true, true, false, false, "DET"},
    {"<", false, true, false, false, "OPE"},
    {">", false, true, false, false, "OPE"},
    {"<=", false, true, false, false, "OPE"},
    {">=", false, true, false, false, "OPE"},

    /* Pattern matching */
    {"LIKE", false, false, true, true, "SEARCH"},
    {"NOT LIKE", false, false, true, true, "SEARCH"},
    {"ILIKE", false, false, true, true, "SEARCH"},
    {"NOT ILIKE", false, false, true, true, "SEARCH"},

    /* NULL-safe comparison (IS DISTINCT FROM ≡ <>, IS NOT DISTINCT FROM ≡ =) */
    {"IS DISTINCT FROM", true, true, false, false, "DET"},
    {"IS NOT DISTINCT FROM", true, true, false, false, "DET"},

    /* NULL checking */
    {"IS NULL", false, false, false, false, NULL},
    {"IS NOT NULL", false, false, false, false, NULL},

    /* ReqID:TECC4.0 - Homomorphic encryption operators (server-side only) */
    {"+", false, false, false, false, "HE"},
    {"-", false, false, false, false, "HE"},
    {"*", false, false, false, false, "HE"},
    /* ReqID:TECC4.0 - Homomorphic power (^) is a HE operator like * :
     * exponent is a plain constant, server-side repeated FF multiplication. */
    {"^", false, false, false, false, "HE"},

    {NULL, false, false, false, false, NULL}
};

/*
 * where_get_column_dek_from_expr
 *      Resolve the DEK name of the encrypted column referenced by expr
 *      (ColumnRef, possibly wrapped in CollateClause).  Returns NULL when
 *      the column is not encrypted or cannot be resolved.
 */
static char *
where_get_column_dek_from_expr(Node *expr, TeccStatementData *statement_data)
{
    char *column_name = NULL;
    char *table_name = NULL;
    const char *schema_name = "public";
    Node *col_ref_node = expr;
    TeccProcessorColumnRef *found_ed = NULL;
    char *dek_name = NULL;

    if (nodeTag(col_ref_node) == FE_T_CollateClause)
        col_ref_node = ((CollateClause *) col_ref_node)->arg;

    if (nodeTag(col_ref_node) != FE_T_ColumnRef)
        return NULL;

    {
        struct ColumnRef *col_ref = (struct ColumnRef *) col_ref_node;
        int num_fields;

        if (col_ref->fields == NIL)
            return NULL;
        column_name = strVal((Value *) llast(col_ref->fields));
        if (column_name == NULL)
            return NULL;
        num_fields = list_length(col_ref->fields);
        if (num_fields >= 2)
            table_name = strVal((Value *) list_nth(col_ref->fields, num_fields - 2));
        if (num_fields >= 3)
            schema_name = strVal((Value *) linitial(col_ref->fields));
    }

    /* Resolve real table name from statement encrypted_columns (alias-aware) */
    if (statement_data->encrypted_columns != NIL)
    {
        ListCell *lc;
        int match_cnt = 0;

        foreach(lc, statement_data->encrypted_columns)
        {
            TeccProcessorColumnRef *ed = (TeccProcessorColumnRef *) lfirst(lc);

            if (ed == NULL || ed->column_name[0] == '\0')
                continue;
            if (strcmp(ed->column_name, column_name) != 0)
                continue;
            if (table_name != NULL)
            {
                if (strcmp(ed->table_alias, table_name) == 0 ||
                    strcmp(ed->table_name, table_name) == 0)
                {
                    found_ed = ed;
                    match_cnt = 1;
                    break;
                }
            }
            else
            {
                found_ed = ed;
                match_cnt++;
            }
        }
        if (found_ed != NULL && match_cnt == 1)
        {
            table_name = found_ed->table_name;
            schema_name = found_ed->schema_name[0] != '\0' ? found_ed->schema_name : schema_name;
        }
    }

    if (table_name == NULL)
        return NULL;

    dek_name = tecc_get_column_dek_name(statement_data, schema_name, table_name, column_name);
    return dek_name;
}

/*
 * where_load_sublink_tables
 *      Load encrypted columns of tables referenced in the subquery's FROM
 *      clause into statement_data->encrypted_columns, so that DEK resolution
 *      (where_get_column_dek_from_expr) works for subquery target columns
 *      (e.g. v IN (SELECT e.v FROM t_e e)) and correlated comparisons inside
 *      the subquery (e.g. EXISTS ... WHERE e.v = a.v).
 */
static void
where_load_sublink_tables(struct SubLink *sl, TeccStatementData *statement_data)
{
    struct SelectStmt *ss;
    ListCell *lc;

    if (sl == NULL || sl->subselect == NULL ||
        nodeTag(sl->subselect) != FE_T_SelectStmt)
        return;

    ss = (struct SelectStmt *) sl->subselect;
    foreach(lc, ss->fromClause)
    {
        Node *from_item = (Node *) lfirst(lc);
        TeccColumnCacheItem **table_cols = NULL;
        int table_col_count = 0;
        int ci;
        TeccState *state;
        const char *db_name;
        RangeVar *rv;

        if (nodeTag(from_item) != FE_T_RangeVar)
            continue;
        rv = (RangeVar *) from_item;
        if (rv->relname == NULL)
            continue;

        state = statement_data->conn ? tecc_get_state(statement_data->conn) : NULL;
        if (state == NULL)
            continue;
        db_name = statement_data->conn ? PQdb(statement_data->conn) : "postgres";
        table_cols = tecc_cache_get_columns_by_table_name(state, db_name,
                    rv->schemaname ? rv->schemaname : "public", rv->relname,
                    &table_col_count);
        if (table_cols == NULL || table_col_count <= 0)
            continue;
        for (ci = 0; ci < table_col_count; ci++)
        {
            TeccProcessorColumnRef *cd;

            if (table_cols[ci] == NULL)
                continue;
            cd = (TeccProcessorColumnRef *)tecc_column_ref_data_create();
            if (cd == NULL)
                continue;
            tecc_column_ref_data_init((TeccColumnRefData *)cd, NULL,
                                      rv->schemaname,
                                      rv->relname,
                                      table_cols[ci]->column_name);
            if (rv->alias != NULL && rv->alias->aliasname != NULL)
                tecc_column_ref_data_set_alias((TeccColumnRefData *)cd, rv->alias->aliasname);
            cd->is_encrypted = true;
            statement_data->encrypted_columns = lappend(
                statement_data->encrypted_columns, cd);
        }
        pg_free(table_cols);
    }
}

/*
 * where_check_sublink_dek
 *      Reject IN/ANY/ALL subquery comparisons where the subquery target
 *      column is encrypted with a different DEK than the outer column
 *      (otherwise the comparison would silently return wrong results).
 */
static TeccWhereClauseProcessorResult
where_check_sublink_dek(struct SubLink *sl, Node *outer_expr,
                        TeccStatementData *statement_data)
{
    char *outer_dek = NULL;
    struct SelectStmt *ss;
    ListCell *tlc;

    if (sl == NULL || sl->subselect == NULL ||
        nodeTag(sl->subselect) != FE_T_SelectStmt)
        return TECC_WHERE_PROCESSOR_SUCCESS;

    outer_dek = where_get_column_dek_from_expr(outer_expr, statement_data);
    if (outer_dek == NULL)
        return TECC_WHERE_PROCESSOR_SUCCESS;

    ss = (struct SelectStmt *) sl->subselect;
    foreach(tlc, ss->targetList)
    {
        ResTarget *rt = (ResTarget *) lfirst(tlc);
        char *inner_dek;

        if (rt == NULL || rt->val == NULL || nodeTag(rt->val) != FE_T_ColumnRef)
            continue;
        inner_dek = where_get_column_dek_from_expr(rt->val, statement_data);
        if (inner_dek != NULL && strcmp(outer_dek, inner_dek) != 0)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "Cannot compare encrypted columns with different DEKs: %s vs %s",
                               outer_dek, inner_dek);
            tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                                   "cannot compare encrypted columns with different DEKs: \"%s\" and \"%s\"",
                                   outer_dek, inner_dek);
            if (outer_dek)
                pg_free(outer_dek);
            if (inner_dek)
                pg_free(inner_dek);
            return TECC_WHERE_PROCESSOR_DEK_MISMATCH;
        }
        if (inner_dek)
            pg_free(inner_dek);
    }
    if (outer_dek)
        pg_free(outer_dek);
    return TECC_WHERE_PROCESSOR_SUCCESS;
}

/*
 * Main WHERE clause processing entry point
 */
TeccWhereClauseProcessorResult
tecc_where_clause_processor_process(Node *where_clause,
                                   TeccStatementData *statement_data)
{
    TeccWhereClauseProcessorContext context;
    TeccWhereClauseProcessorResult result;

    if (statement_data == NULL)
        return TECC_WHERE_PROCESSOR_ERROR;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Starting WHERE clause processing");

    /* Check if WHERE clause exists */
    if (where_clause == NULL)
    {

        return TECC_WHERE_PROCESSOR_SUCCESS;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "WHERE clause type=%d", nodeTag(where_clause));

    /* Initialize context */
    memset(&context, 0, sizeof(context));
    context.encrypted_columns = statement_data->encrypted_columns;
    context.processed_conditions = NIL;
    context.needs_rewrite = false;
    context.operator_count = 0;
    context.encrypted_operator_count = 0;

    /* Process the WHERE clause expression */
    result = where_process_expr(where_clause, statement_data);

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "WHERE clause processing result=%d", result);

    if (result == TECC_WHERE_PROCESSOR_SUCCESS)
    {


        /* Update statement data if rewrite is needed */
        if (context.needs_rewrite)
        {
            statement_data->needs_rewrite = 1;

        }
    }

    return result;
}

/*
 * Process individual condition
 */
TeccWhereClauseProcessorResult
tecc_where_clause_processor_process_condition(Node *condition,
                                             TeccStatementData *statement_data)
{
    if (condition == NULL)
        return TECC_WHERE_PROCESSOR_SUCCESS;

    return where_process_expr(condition, statement_data);
}

/*
 * Process an arbitrary expression node for constant encryption.
 * This is a public wrapper around where_process_expr(), useful for
 * SELECT target list expressions, RETURNING clauses, etc.
 */
TeccWhereClauseProcessorResult
tecc_where_clause_processor_process_expr(Node *expr,
                                         TeccStatementData *statement_data)
{
    if (expr == NULL)
        return TECC_WHERE_PROCESSOR_SUCCESS;

    return where_process_expr(expr, statement_data);
}

/*
 * Check operator compatibility
 */
TeccWhereClauseProcessorResult
tecc_where_clause_processor_check_operator(const char *operator_name,
                                          const char *left_type,
                                          const char *right_type,
                                          TeccStatementData *statement_data)
{
    const TeccOperatorSupport *support;

    if (operator_name == NULL)
    {
        tecc_processor_set_error(statement_data, TECC_PROCESSOR_INVALID_INPUT,
                                "Operator name is NULL");
        return TECC_WHERE_PROCESSOR_ERROR;
    }

    /* Get operator support information */
    support = where_get_operator_support(operator_name);
    if (support == NULL)
    {
        
        return TECC_WHERE_PROCESSOR_UNSUPPORTED_OPERATOR;
    }

    /* Check if types are compatible */
    if (left_type != NULL && right_type != NULL)
    {
        /* Basic type compatibility checking */
        bool types_compatible = false;

        /* Check if types are the same */
        if (strcmp(left_type, right_type) == 0)
        {
            types_compatible = true;
        }
        /* Check numeric type compatibility */
        else if ((strcmp(left_type, "int2_enc") == 0 || strcmp(left_type, "int4_enc") == 0 ||
                  strcmp(left_type, "int8_enc") == 0 || strcmp(left_type, "float4_enc") == 0 ||
                  strcmp(left_type, "float8_enc") == 0 || strcmp(left_type, "numeric_enc") == 0) &&
                 (strcmp(right_type, "int2_enc") == 0 || strcmp(right_type, "int4_enc") == 0 ||
                  strcmp(right_type, "int8_enc") == 0 || strcmp(right_type, "float4_enc") == 0 ||
                  strcmp(right_type, "float8_enc") == 0 || strcmp(right_type, "numeric_enc") == 0))
        {
            /* Numeric types are generally compatible with each other */
            types_compatible = true;
        }
        /* Check string type compatibility */
        else if ((strcmp(left_type, "varchar_enc") == 0 || strcmp(left_type, "bpchar_enc") == 0 ||
                  strcmp(left_type, "text_enc") == 0) &&
                 (strcmp(right_type, "varchar_enc") == 0 || strcmp(right_type, "bpchar_enc") == 0 ||
                  strcmp(right_type, "text_enc") == 0))
        {
            /* String types are generally compatible with each other */
            types_compatible = true;
        }

        if (!types_compatible)
        {
            tecc_processor_debug(TECC_LOG_WARNING,
                                "Type mismatch: %s %s %s", left_type, operator_name, right_type);
            tecc_processor_set_error(statement_data, TECC_PROCESSOR_INVALID_INPUT,
                                    "Type mismatch in operator expression");
            return TECC_WHERE_PROCESSOR_INCOMPATIBLE_TYPES;
        }

        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "Type compatibility check passed: %s %s %s",
                            left_type, operator_name, right_type);
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Operator %s is supported with algorithm %s",
                        operator_name,
                        support->required_algorithm ? support->required_algorithm : "none");

    return TECC_WHERE_PROCESSOR_SUCCESS;
}

/*
 * Get supported algorithms for operator
 */
List *
tecc_where_clause_processor_get_supported_algorithms(const char *operator_name)
{
    List *algorithms;
    const TeccOperatorSupport *support;

    if (operator_name == NULL)
        return NIL;

    algorithms = NIL;
    support = where_get_operator_support(operator_name);

    if (support != NULL)
    {
        /* Add supported algorithms to the list */
        algorithms = lappend(algorithms, makeStringConst("DET", 0, 0));
        algorithms = lappend(algorithms, makeStringConst("OPE", 0, 0));
        algorithms = lappend(algorithms, makeStringConst("SEARCH", 0, 0));
    }

    return algorithms;
}

/*
 * Check if operator needs encryption
 */
bool
tecc_where_clause_processor_operator_needs_encryption(const char *operator_name,
                                                     const char *column_type)
{
    const TeccOperatorSupport *support;

    if (operator_name == NULL || column_type == NULL)
        return false;

    /* Check if column type indicates encryption */
    if (strstr(column_type, "_enc") == NULL)
        return false;

    /* Get operator support */
    support = where_get_operator_support(operator_name);
    if (support == NULL)
        return false;

    /* Operator needs encryption if it requires an algorithm */
    return (support->required_algorithm != NULL);
}

/*
 * ReqID:TECC4.0 - Extract first ColumnRef from an expression tree.
 * Recursively searches common node types (NullTest, TypeCast, CaseExpr,
 * CaseWhen, A_Expr, RowExpr) to find a ColumnRef for DEK lookup.
 */
static struct ColumnRef *
extract_first_column_ref_from_node(Node *node)
{
    struct ColumnRef *cr;
    ListCell *lc;

    if (node == NULL)
        return NULL;

    if (nodeTag(node) == FE_T_ColumnRef)
        return (struct ColumnRef *) node;

    if (nodeTag(node) == FE_T_NullTest)
    {
        NullTest *nt = (NullTest *) node;
        return extract_first_column_ref_from_node(nt->arg);
    }

    /* ReqID:TECC4.0 - BooleanTest: unwrap arg to find column ref */
    if (nodeTag(node) == FE_T_BooleanTest)
    {
        struct BooleanTest *bt = (struct BooleanTest *) node;
        return extract_first_column_ref_from_node(bt->arg);
    }

    /* ReqID:TECC4.0 - TimeZoneExpr: unwrap arg to find column ref */
    if (nodeTag(node) == FE_T_TimeZoneExpr)
    {
        struct TimeZoneExpr *tz = (struct TimeZoneExpr *) node;
        return extract_first_column_ref_from_node(tz->arg);
    }

    if (nodeTag(node) == FE_T_TypeCast)
    {
        TypeCast *tc = (TypeCast *) node;
        return extract_first_column_ref_from_node(tc->arg);
    }

    /* ReqID:TECC4.0 - A_Indirection: unwrap arg to find column ref */
    if (nodeTag(node) == FE_T_A_Indirection)
    {
        struct A_Indirection *ai = (struct A_Indirection *) node;
        cr = extract_first_column_ref_from_node(ai->arg);
        if (cr != NULL) return cr;
    }

    if (nodeTag(node) == FE_T_CaseExpr)
    {
        CaseExpr *ce = (CaseExpr *) node;
        /* ReqID:TECC4.0 - Only recurse into result branches (cw->result,
         * ce->defresult); do NOT recurse into cw->expr (the WHEN condition).
         * Extracting a column ref from the condition and using it to encrypt
         * the outer comparison value produces wrong type resolution. E.g.:
         *   WHERE CASE WHEN val > 150 THEN 1 ELSE 0 END = 1
         * The outer =1 must NOT pick up 'val' from the condition. */
        foreach(lc, ce->args)
        {
            CaseWhen *cw = (CaseWhen *) lfirst(lc);
            if (cw != NULL)
            {
                cr = extract_first_column_ref_from_node(cw->result);
                if (cr != NULL) return cr;
            }
        }
        return extract_first_column_ref_from_node(ce->defresult);
    }

    if (nodeTag(node) == FE_T_A_Expr)
    {
        A_Expr *a = (A_Expr *) node;
        cr = extract_first_column_ref_from_node(a->lexpr);
        if (cr != NULL) return cr;
        cr = extract_first_column_ref_from_node(a->rexpr);
        if (cr != NULL) return cr;
        if (a->escape != NULL)
            return extract_first_column_ref_from_node(a->escape);
        return NULL;
    }

    if (nodeTag(node) == FE_T_RowExpr)
    {
        RowExpr *re = (RowExpr *) node;
        foreach(lc, re->args)
        {
            cr = extract_first_column_ref_from_node((Node *) lfirst(lc));
            if (cr != NULL) return cr;
        }
    }

    /* ReqID:TECC4.0 - CollateClause: unwrap to find column ref underneath */
    if (nodeTag(node) == FE_T_CollateClause)
    {
        CollateClause *cc = (CollateClause *) node;
        return extract_first_column_ref_from_node(cc->arg);
    }

    return NULL;
}

/*
 * Process comparison expression
 */
TeccWhereClauseProcessorResult
tecc_where_clause_processor_process_comparison(Node *left_expr,
                                              Node *right_expr,
                                              const char *operator_name,
                                              TeccStatementData *statement_data)
{
    TeccWhereClauseProcessorResult result;
    bool left_is_encrypted = false;
    bool right_is_encrypted = false;
    char *left_column = NULL;
    char *right_column = NULL;
    /* Variables for encryption scheduling */
    const char *table_name = NULL;
    const char *column_name = NULL;
    const char *schema_name = NULL;
    const char *kek_name = NULL;
    const char *dek_name = NULL;
    char *actual_dek_name = NULL;
    TeccUnifiedEncryptResult *encrypt_result = NULL;
    /* Variables for encrypted value storage */
    TeccRawValue *raw_value = NULL;

    if (left_expr == NULL || right_expr == NULL || operator_name == NULL)
        return TECC_WHERE_PROCESSOR_ERROR;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing comparison: %s %s %s",
                        nodeTag(left_expr) == FE_T_ColumnRef ? "column" : "expr",
                        operator_name,
                        nodeTag(right_expr) == FE_T_ColumnRef ? "column" : "expr");

    /* Check if left side is an encrypted column or contains one (e.g. FuncCall) */
    if (nodeTag(left_expr) == FE_T_ColumnRef)
    {
        struct ColumnRef *col_ref = (struct ColumnRef *) left_expr;
        if (col_ref->fields != NIL)
        {
            /* Get last field (column name) from the list */
            int num_fields = list_length(col_ref->fields);
            Node *field = (Node *) list_nth(col_ref->fields, num_fields - 1);
            left_column = strVal((Value *)field);
            if (left_column != NULL)
            {
                left_is_encrypted = where_is_encrypted_column(left_column, statement_data);
            }
        }
    }
    else if (nodeTag(left_expr) == FE_T_FuncCall)
    {
        /* ReqID:TECC4.0 - FuncCall may contain encrypted column args */
        FuncCall *fc = (FuncCall *) left_expr;
        ListCell *alc;
        foreach(alc, fc->args)
        {
            Node *arg = (Node *) alc->data.ptr_value;
            if (arg != NULL && nodeTag(arg) == FE_T_ColumnRef)
            {
                struct ColumnRef *cr = (struct ColumnRef *) arg;
                if (cr->fields != NIL)
                {
                    int nf = list_length(cr->fields);
                    Node *f = (Node *) list_nth(cr->fields, nf - 1);
                    left_column = strVal((Value *) f);
                    if (left_column != NULL &&
                        where_is_encrypted_column(left_column, statement_data))
                    {
                        left_is_encrypted = true;
                        break;
                    }
                }
            }
        }
    }
    else if (nodeTag(left_expr) == FE_T_CaseExpr)
    {
        /* ReqID:TECC4.0 - CaseExpr may contain encrypted column in branches.
         * Use recursive search to handle nesting (e.g. CASE WHEN val>150 THEN...). */
        struct ColumnRef *found_cr;
        found_cr = extract_first_column_ref_from_node(left_expr);
        if (found_cr != NULL && found_cr->fields != NIL)
        {
            int nf;
            Node *f;
            nf = list_length(found_cr->fields);
            f = (Node *) list_nth(found_cr->fields, nf - 1);
            left_column = strVal((Value *) f);
            if (left_column != NULL &&
                where_is_encrypted_column(left_column, statement_data))
                left_is_encrypted = true;
        }
    }
    /* ReqID:TECC4.0 - CollateClause wraps a column ref (e.g. name COLLATE "C") */
    else if (nodeTag(left_expr) == FE_T_CollateClause)
    {
        struct ColumnRef *found_cr;
        found_cr = extract_first_column_ref_from_node(left_expr);
        if (found_cr != NULL && found_cr->fields != NIL)
        {
            int nf;
            Node *f;
            nf = list_length(found_cr->fields);
            f = (Node *) list_nth(found_cr->fields, nf - 1);
            left_column = strVal((Value *) f);
            if (left_column != NULL &&
                where_is_encrypted_column(left_column, statement_data))
                left_is_encrypted = true;
        }
    }

    /* Check if right side is an encrypted column or contains one */
    if (nodeTag(right_expr) == FE_T_ColumnRef)
    {
        struct ColumnRef *col_ref = (struct ColumnRef *) right_expr;
        if (col_ref->fields != NIL)
        {
            /* Get last field (column name) from the list */
            int num_fields = list_length(col_ref->fields);
            Node *field = (Node *) list_nth(col_ref->fields, num_fields - 1);
            right_column = strVal((Value *)field);
            if (right_column != NULL)
            {
                right_is_encrypted = where_is_encrypted_column(right_column, statement_data);
            }
        }
    }
    else if (nodeTag(right_expr) == FE_T_FuncCall)
    {
        /* ReqID:TECC4.0 - FuncCall may contain encrypted column args */
        FuncCall *fc = (FuncCall *) right_expr;
        ListCell *alc;
        foreach(alc, fc->args)
        {
            Node *arg = (Node *) alc->data.ptr_value;
            if (arg != NULL && nodeTag(arg) == FE_T_ColumnRef)
            {
                struct ColumnRef *cr = (struct ColumnRef *) arg;
                if (cr->fields != NIL)
                {
                    int nf = list_length(cr->fields);
                    Node *f = (Node *) list_nth(cr->fields, nf - 1);
                    right_column = strVal((Value *) f);
                    if (right_column != NULL &&
                        where_is_encrypted_column(right_column, statement_data))
                    {
                        right_is_encrypted = true;
                        break;
                    }
                }
            }
        }
    }
    else if (nodeTag(right_expr) == FE_T_CaseExpr)
    {
        /* ReqID:TECC4.0 - CaseExpr may contain encrypted column in branches.
         * Use recursive search to handle nesting (e.g. CASE WHEN val>150 THEN...). */
        struct ColumnRef *found_cr;
        found_cr = extract_first_column_ref_from_node(right_expr);
        if (found_cr != NULL && found_cr->fields != NIL)
        {
            int nf;
            Node *f;
            nf = list_length(found_cr->fields);
            f = (Node *) list_nth(found_cr->fields, nf - 1);
            right_column = strVal((Value *) f);
            if (right_column != NULL &&
                where_is_encrypted_column(right_column, statement_data))
                right_is_encrypted = true;
        }
    }
    /* ReqID:TECC4.0 - CollateClause wraps a column ref (e.g. = 'val' COLLATE "C") */
    else if (nodeTag(right_expr) == FE_T_CollateClause)
    {
        struct ColumnRef *found_cr;
        found_cr = extract_first_column_ref_from_node(right_expr);
        if (found_cr != NULL && found_cr->fields != NIL)
        {
            int nf;
            Node *f;
            nf = list_length(found_cr->fields);
            f = (Node *) list_nth(found_cr->fields, nf - 1);
            right_column = strVal((Value *) f);
            if (right_column != NULL &&
                where_is_encrypted_column(right_column, statement_data))
                right_is_encrypted = true;
        }
    }

    /* Check operator compatibility with encrypted columns */
    if (left_is_encrypted || right_is_encrypted)
    {
        /* ReqID:TECC4.0 - Both sides are encrypted columns: reject comparing
         * columns encrypted with different DEKs (otherwise the comparison
         * would silently return wrong results, e.g. an equality JOIN on
         * different DEKs returning no rows). */
        if (left_is_encrypted && right_is_encrypted)
        {
            char *left_dek;
            char *right_dek;
            /* ReqID:TECC4.0 - LIKE/NOT LIKE/ILIKE between two encrypted
             * columns is not supported: the frontend encrypts LIKE patterns
             * differently from plain data columns, so a ciphertext column
             * cannot serve as a LIKE pattern. Reject explicitly instead of
             * silently comparing SEARCH segments. */
            if (strcmp(operator_name, "LIKE") == 0 ||
                strcmp(operator_name, "NOT LIKE") == 0 ||
                strcmp(operator_name, "ILIKE") == 0 ||
                strcmp(operator_name, "NOT ILIKE") == 0)
            {
                tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                                       "LIKE/NOT LIKE between two encrypted columns is not supported. "
                                       "The LIKE pattern must be a plain string constant.");
                return TECC_WHERE_PROCESSOR_LIKE_COLUMN;
            }

            /* ReqID:TECC4.0 - A15: HE arithmetic operators (*, +, -, ^) skip
             * frontend cross-DEK check.  Server-side HE operators validate
             * ciphertext slot compatibility (EG_ADD/EG_MUL) and report
             * precise "algorithm not supported" errors.  Frontend DEK check
             * only applies to comparison operators (=, <, >, etc.) where
             * cross-DEK would silently return wrong results. */
            if (strcmp(operator_name, "*") != 0 &&
                strcmp(operator_name, "+") != 0 &&
                strcmp(operator_name, "-") != 0 &&
                strcmp(operator_name, "^") != 0)
            {
                left_dek = where_get_column_dek_from_expr(left_expr, statement_data);
                right_dek = where_get_column_dek_from_expr(right_expr, statement_data);

                if (left_dek != NULL && right_dek != NULL && strcmp(left_dek, right_dek) != 0)
                {
                    tecc_processor_debug(TECC_LOG_ERROR,
                                       "Cannot compare encrypted columns with different DEKs: %s vs %s",
                                       left_dek, right_dek);
                    tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                                           "cannot compare encrypted columns with different DEKs: \"%s\" and \"%s\"",
                                           left_dek, right_dek);
                    if (left_dek)
                        pg_free(left_dek);
                    if (right_dek)
                        pg_free(right_dek);
                    return TECC_WHERE_PROCESSOR_DEK_MISMATCH;
                }
                if (left_dek)
                    pg_free(left_dek);
                if (right_dek)
                    pg_free(right_dek);
            }
        }

        result = tecc_where_clause_processor_check_operator(operator_name,
                                                           left_is_encrypted ? "text_enc" : NULL,
                                                           right_is_encrypted ? "text_enc" : NULL,
                                                           statement_data);
        if (result != TECC_WHERE_PROCESSOR_SUCCESS)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "Operator %s not compatible with encrypted columns",
                               operator_name);
            return result;
        }

        /* Set operator type for SEARCH algorithm interface extension */
        tecc_where_clause_processor_set_operator_type(statement_data, operator_name);

        /* Mark that rewrite is needed */
        

        /* Begin - ReqID:TECC4.0 - 算法兼容性检查 */
        /* Check if the encrypted column's algorithm supports this operator.
         * ReqID:TECC4.0 - P2-4: also enter for UMINUS+Const (negative
         * constants like -5 are parsed as A_Expr(UMINUS, Const)), so the
         * HE scalar-operator skip below covers `a + (-5)` as well. */
        if ((left_is_encrypted &&
             (nodeTag(right_expr) == FE_T_Const ||
              (nodeTag(right_expr) == FE_T_A_Expr &&
               ((A_Expr *) right_expr)->kind == AEXPR_OP &&
               (((A_Expr *) right_expr)->lexpr == NULL ||
                ((A_Expr *) right_expr)->rexpr == NULL)))) ||
            (right_is_encrypted &&
             (nodeTag(left_expr) == FE_T_Const ||
              (nodeTag(left_expr) == FE_T_A_Expr &&
               ((A_Expr *) left_expr)->kind == AEXPR_OP &&
               (((A_Expr *) left_expr)->lexpr == NULL ||
                ((A_Expr *) left_expr)->rexpr == NULL)))))
        {
            /* Get the column name that is encrypted */
            const char *check_col_name = left_is_encrypted ? left_column : right_column;
            const char *check_table_name = NULL;
            const char *check_schema_name = "public";
            Node *col_ref_node = left_is_encrypted ? left_expr : right_expr;
            TeccColumnCacheItem *col_item = NULL;
            TeccState *state = NULL;

            /* Try to resolve table name from ColumnRef or CaseExpr */
            if (nodeTag(col_ref_node) == FE_T_ColumnRef)
            {
                struct ColumnRef *col_ref = (struct ColumnRef *) col_ref_node;
                if (col_ref->fields != NIL)
                {
                    int num_fields = list_length(col_ref->fields);
                    if (num_fields >= 2)
                    {
                        Node *table_field = (Node *) list_nth(col_ref->fields, num_fields - 2);
                        check_table_name = strVal((Value *)table_field);
                    }
                    if (num_fields >= 3)
                    {
                        Node *schema_field = (Node *) list_nth(col_ref->fields, num_fields - 3);
                        check_schema_name = strVal((Value *)schema_field);
                    }
                }
            }
            else if (nodeTag(col_ref_node) == FE_T_CaseExpr)
            {
                /* ReqID:TECC4.0 - Extract ColumnRef from CaseExpr for algorithm check */
                struct ColumnRef *found_cr;
                int found_nf;
                found_cr = extract_first_column_ref_from_node(col_ref_node);
                if (found_cr != NULL && found_cr->fields != NIL)
                {
                    found_nf = list_length(found_cr->fields);
                    if (found_nf >= 2)
                    {
                        Node *table_field;
                        table_field = (Node *) list_nth(found_cr->fields, found_nf - 2);
                        check_table_name = strVal((Value *)table_field);
                    }
                    if (found_nf >= 3)
                    {
                        Node *schema_field;
                        schema_field = (Node *) list_nth(found_cr->fields, found_nf - 3);
                        check_schema_name = strVal((Value *)schema_field);
                    }
                }
            }
            /* ReqID:TECC4.0 - CollateClause wraps a column ref */
            else if (nodeTag(col_ref_node) == FE_T_CollateClause)
            {
                struct ColumnRef *found_cr;
                int found_nf;
                found_cr = extract_first_column_ref_from_node(col_ref_node);
                if (found_cr != NULL && found_cr->fields != NIL)
                {
                    found_nf = list_length(found_cr->fields);
                    if (found_nf >= 2)
                    {
                        Node *table_field;
                        table_field = (Node *) list_nth(found_cr->fields, found_nf - 2);
                        check_table_name = strVal((Value *)table_field);
                    }
                    if (found_nf >= 3)
                    {
                        Node *schema_field;
                        schema_field = (Node *) list_nth(found_cr->fields, found_nf - 3);
                        check_schema_name = strVal((Value *)schema_field);
                    }
                }
            }

            /* ReqID:TECC4.0 - Resolve table name from encrypted_columns list.
             * ColumnRef may contain a table alias rather than the real name.
             * If check_table_name is set from ColumnRef, match by both alias/table AND column.
             * If check_table_name is NULL, require unique column name across all tables. */
            if (check_col_name != NULL && statement_data->encrypted_columns != NIL)
            {
                ListCell *lc;
                TeccColumnRefData *found_ed = NULL;
                int ed_match_count = 0;

                foreach(lc, statement_data->encrypted_columns)
                {
                    TeccColumnRefData *ed = (TeccColumnRefData *) lfirst(lc);
                    if (ed == NULL || ed->column_name[0] == '\0')
                        continue;
                    /* Bug 9323 - 重命名后的输出列按 column_alias 匹配 */
                    if (strcmp(ed->column_name, check_col_name) != 0 &&
                        (ed->column_alias[0] == '\0' ||
                         strcmp(ed->column_alias, check_col_name) != 0))
                        continue;

                    if (check_table_name != NULL)
                    {
                        /* Strict: match table_alias or table_name from encrypted_columns */
                        if (strcmp(ed->table_alias, check_table_name) == 0 ||
                            strcmp(ed->table_name, check_table_name) == 0)
                        {
                            found_ed = ed;
                            ed_match_count = 1;
                            break;
                        }
                    }
                    else
                    {
                        found_ed = ed;
                        ed_match_count++;
                    }
                }

                if (found_ed != NULL && ed_match_count == 1)
                {
                    check_table_name = found_ed->table_name;
                    /* Bug 9323 - 命中可能来自 column_alias，回填底层列名 */
                    check_col_name = found_ed->column_name;
                    check_schema_name = "public";
                }
                else if (ed_match_count > 1)
                {
                    if (statement_data->multi_table_context)
                    {
                        tecc_processor_debug(TECC_LOG_ERROR,
                                           "Ambiguous encrypted column \"%s\" in algorithm check: "
                                           "found in multiple tables. Use qualified name.",
                                           check_col_name);
                    }
                    else
                    {
                        /* Backward compatible: use first match */
                        check_table_name = found_ed->table_name;
                        check_schema_name = "public";
                    }
                }
            }

            /* Get TeccState for cache lookup */
            if (statement_data->conn != NULL)
            {
                state = tecc_get_state(statement_data->conn);
            }

            /* Look up column cache to get encryption algorithm info */
            if (state != NULL && check_table_name != NULL && check_col_name != NULL)
            {
                const char *db_name = (statement_data->conn != NULL) ? PQdb(statement_data->conn) : NULL;
                if (db_name == NULL || db_name[0] == '\0')
                    db_name = "postgres";
                col_item = tecc_cache_get_encrypted_column_by_name(
                    state, db_name, check_schema_name, check_table_name, check_col_name);
            }
            else
            {
            }

            /* If cache lookup succeeded, check algorithm compatibility */
            if (col_item != NULL)
            {
                result = where_check_algorithm_for_operator(operator_name, col_item, statement_data);
                if (result != TECC_WHERE_PROCESSOR_SUCCESS)
                {
                    return result;
                }

                /* ReqID:TECC4.0 - HE 标量运算常量客户端加密：
                 * + / - 需要 he-add（EC-ElGamal）列，* 需要 he-mul（FF ElGamal）
                 * 列；常量在前端用列的公钥同态加密成密文，改写为
                 * `enc + 'hex'::intN_enc` 发往后端做全密文同态运算，
                 * 常量明文不再出现在网络报文与服务端（he_add_generic /
                 * he_sub_generic / he_mul_generic 处理密文×密文）。
                 *
                 * 例外（保持透传，服务端按现状处理）：
                 *   - 列仅 he-add 时的 *：EC-ElGamal 标量乘在数学上必须知道
                 *     明文 k（E(a)×k 无法由 E(k) 计算），保持服务端明文标量乘；
                 *   - ^ 幂运算的指数：服务端重复乘法的循环次数，必须为明文；
                 *   - 常量类型非整数（int2/int4/int8）：不支持 HE 常量加密，
                 *     保持透传（与现状行为一致）。 */
                if (strcmp(operator_name, "+") == 0 ||
                    strcmp(operator_name, "-") == 0 ||
                    strcmp(operator_name, "*") == 0 ||
                    strcmp(operator_name, "^") == 0)
                {
                    bool he_add_col = (col_item->encryption_type & 0x08) != 0;
                    bool he_mul_col = (col_item->encryption_type & 0x10) != 0;
                    bool skip_he_encrypt = false;

                    if (strcmp(operator_name, "+") == 0 ||
                        strcmp(operator_name, "-") == 0)
                    {
                        /* he-add 列：前端 EC-ElGamal 加密常量（EG_ADD 段）。
                         * 列无 he-add 时透传（服务端报错，与现状一致）。 */
                        if (!he_add_col)
                            skip_he_encrypt = true;
                    }
                    else if (strcmp(operator_name, "*") == 0)
                    {
                        /* he-mul 列：前端 FF ElGamal 加密常量（EG_MUL 段）。
                         * 仅 he-add 列（EC 标量乘需明文 k）透传。 */
                        if (!he_mul_col)
                            skip_he_encrypt = true;
                    }
                    else
                    {
                        /* ^ 幂运算：指数保持明文（服务端循环次数） */
                        skip_he_encrypt = true;
                    }

                    if (!skip_he_encrypt)
                    {
                        /* 常量类型必须是整数，否则透传（与现状一致） */
                        Node *he_const_node = (left_is_encrypted) ? right_expr : left_expr;
                        Oid he_const_type = InvalidOid;

                        if (nodeTag(he_const_node) == FE_T_Const)
                        {
                            he_const_type = ((Const *)he_const_node)->consttype;
                        }
                        else if (nodeTag(he_const_node) == FE_T_A_Expr)
                        {
                            /* UMINUS+Const：取内部 Const 类型 */
                            A_Expr *he_ae = (A_Expr *)he_const_node;
                            Node *he_inner = he_ae->rexpr ? he_ae->rexpr : he_ae->lexpr;
                            if (he_inner != NULL && nodeTag(he_inner) == FE_T_Const)
                                he_const_type = ((Const *)he_inner)->consttype;
                        }
                        if (he_const_type != INT2OID &&
                            he_const_type != INT4OID &&
                            he_const_type != INT8OID)
                        {
                            skip_he_encrypt = true;
                        }
                    }

                    if (skip_he_encrypt)
                    {
                        /* 透传：服务端处理明文常量（兼容路径） */
                        return TECC_WHERE_PROCESSOR_SUCCESS;
                    }
                    /* 否则继续走常量加密流程（HE 常量加密分支见下方调度器调用） */
                }
            }
            else
            {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Column %s not found in column cache", check_col_name ? check_col_name : "NULL");
            }
        }
        /* End - ReqID:TECC4.0 - 算法兼容性检查 */
    }

    /* ReqID:TECC4.0 - Handle SubLink on non-encrypted side:
     * when one side is an encrypted column and the other is a SubLink
     * (e.g., salary = (SELECT 50000)), encrypt constants in the
     * SubLink's targetList. */
    if (left_is_encrypted && nodeTag(right_expr) == FE_T_SubLink)
    {
        struct SubLink *sl = (struct SubLink *) right_expr;
        /* ReqID:TECC4.0 - Reject cross-DEK subquery comparisons (e.g. = ANY) */
        result = where_check_sublink_dek(sl, left_expr, statement_data);
        if (result != TECC_WHERE_PROCESSOR_SUCCESS)
            return result;
        where_process_subquery_target_consts(sl->subselect, left_expr,
                                             operator_name, statement_data);
    }
    else if (right_is_encrypted && nodeTag(left_expr) == FE_T_SubLink)
    {
        struct SubLink *sl = (struct SubLink *) left_expr;
        /* ReqID:TECC4.0 - Reject cross-DEK subquery comparisons (e.g. = ANY) */
        result = where_check_sublink_dek(sl, right_expr, statement_data);
        if (result != TECC_WHERE_PROCESSOR_SUCCESS)
            return result;
        where_process_subquery_target_consts(sl->subselect, right_expr,
                                             operator_name, statement_data);
    }

    /* Process value encryption if needed */
    /* ReqID:TECC4.0 - Also handle UMINUS+Const (negative constants like -1
     * are parsed as A_Expr(UMINUS, Const(1))). */
    if ((left_is_encrypted &&
         (nodeTag(right_expr) == FE_T_Const ||
          (nodeTag(right_expr) == FE_T_A_Expr &&
           ((A_Expr *)right_expr)->kind == AEXPR_OP &&
           (((A_Expr *)right_expr)->lexpr == NULL ||
            ((A_Expr *)right_expr)->rexpr == NULL)))) ||
        (right_is_encrypted &&
         (nodeTag(left_expr) == FE_T_Const ||
          (nodeTag(left_expr) == FE_T_A_Expr &&
           ((A_Expr *)left_expr)->kind == AEXPR_OP &&
           (((A_Expr *)left_expr)->lexpr == NULL ||
            ((A_Expr *)left_expr)->rexpr == NULL)))))
    {
        /* Process constant value encryption */
        Node *const_node = (left_is_encrypted) ? right_expr : left_expr;
        Const *const_expr = NULL;
        size_t const_loc = 0, const_dsize = 0;
        char *const_value = NULL;
        const char *enc_query;

        enc_query = statement_data->query;
        /* ReqID:TECC4.0 - Extract const info from Const or UMINUS+Const */
        if (nodeTag(const_node) == FE_T_Const)
        {
            const_expr = (Const *)const_node;
        }

        /* Begin - ReqID:TECC4.0 - 常量值提取逻辑 */
        /* Extract constant value text (works for both Const and UMINUS+Const) */
        if (const_expr != NULL)
        {
            const_value = where_extract_const_value(const_expr, statement_data);
            const_loc = (size_t)const_expr->location;
            const_dsize = (size_t)const_expr->data_size;
        }
        else
        {
            /* Try UMINUS+Const pattern */
            char *dummy_text = NULL;
            Oid dummy_oid;
            if (where_extract_const_info(const_node, enc_query,
                                         &const_loc, &const_dsize,
                                         &dummy_text, &dummy_oid))
            {
                const_value = dummy_text;
                /* const_loc and const_dsize already set */
            }
        }

        if (const_value != NULL)
        {
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Extracted constant value: %s", const_value);
        }
        else
        {
            tecc_processor_debug(TECC_LOG_WARNING,
                               "Failed to extract constant value");
        }
        /* End - ReqID:TECC4.0 - 常量值提取逻辑 */

        if (const_value != NULL)
        {
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Encrypting constant for comparison: column=%s, table=%s, value=%s",
                               column_name, table_name ? table_name : "NULL", const_value);

            /* Begin - ReqID:TECC4.0 - 调用加密调度器 */
            /* Cache module is now available - using tecc_get_column_dek_name and where_get_column_kek_name to get encryption metadata from cache */

            /* Extract table and column information from ColumnRef/FuncCall node */
            {
                Node *col_ref_node = (left_is_encrypted) ? left_expr : right_expr;
                /* Begin - ReqID:TECC4.0 - Bug 7857: unwrap CollateClause to find the underlying ColumnRef */
                if (nodeTag(col_ref_node) == FE_T_CollateClause)
                    col_ref_node = ((CollateClause *)col_ref_node)->arg;
                /* End - ReqID:TECC4.0 */
            if (nodeTag(col_ref_node) == FE_T_ColumnRef)
            {
                struct ColumnRef *col_ref = (struct ColumnRef *) col_ref_node;
                if (col_ref->fields != NIL)
                {
                    /* Get column name from last field */
                    Node *last_field = (Node *) llast(col_ref->fields);
                    column_name = strVal((Value *)last_field);
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                       "WHERE column ref: column=%s, table=%s",
                                       column_name ? column_name : "NULL",
                                       table_name ? table_name : "NULL");
                    if (column_name != NULL)
                    {

                        /* Try to get table name if available */
                        int num_fields = list_length(col_ref->fields);
                        if (num_fields >= 2)
                        {
                            /* Get the second last element (table name) */
                            int second_last_index = num_fields - 2;
                            Node *second_last = (Node *) list_nth(col_ref->fields, second_last_index);
                            table_name = strVal((Value *)second_last);
                        }

                        /* ReqID:TECC4.0 - Resolve real table name from encrypted_columns.
                         * ColumnRef may contain alias, not real table name. */
                        if (statement_data->encrypted_columns != NIL)
                        {
                            ListCell *lc;
                            TeccColumnRefData *found_ed = NULL;
                            int match_cnt = 0;

                            foreach(lc, statement_data->encrypted_columns)
                            {
                                TeccColumnRefData *ed = (TeccColumnRefData *) lfirst(lc);
                                if (ed == NULL || ed->column_name[0] == '\0')
                                    continue;
                                /* Bug 9323 - 子查询/CTE 重命名后的输出列只以别名
                                 * 对外可见，column_name 与 column_alias 任一匹配
                                 * 即算命中。 */
                                if (strcmp(ed->column_name, column_name) != 0 &&
                                    (ed->column_alias[0] == '\0' ||
                                     strcmp(ed->column_alias, column_name) != 0))
                                    continue;

                                if (table_name != NULL)
                                {
                                    /* Match by alias or real table name */
                                    if (strcmp(ed->table_alias, table_name) == 0 ||
                                        strcmp(ed->table_name, table_name) == 0)
                                    {
                                        found_ed = ed;
                                        match_cnt = 1;
                                        break;
                                    }
                                }
                                else
                                {
                                    found_ed = ed;
                                    match_cnt++;
                                }
                            }

                            if (found_ed != NULL && match_cnt == 1)
                            {
                                tecc_processor_debug(TECC_LOG_DEBUG1,
                                                   "Found encrypted column match: %s.%s",
                                                   found_ed->column_name, found_ed->table_name);
                                table_name = found_ed->table_name;
                                schema_name = found_ed->schema_name[0] != '\0' ? found_ed->schema_name : schema_name;
                                /* Bug 9323 - 命中可能是重命名后的输出列
                                 * （column_alias），后续取 DEK/KEK 必须按底层
                                 * 列名查缓存，故这里回填 column_name。 */
                                column_name = found_ed->column_name;
                                tecc_processor_debug(TECC_LOG_DEBUG1,
                                                   "Resolved table name from statement encrypted_columns: %s.%s",
                                                   schema_name ? schema_name : "public", table_name);
                            }
                            else if (match_cnt > 1)
                            {
                                if (statement_data->multi_table_context)
                                {
                                    tecc_processor_debug(TECC_LOG_ERROR,
                                                       "Ambiguous encrypted column \"%s\" - use qualified name",
                                                       column_name);
                                }
                                else if (found_ed != NULL)
                                {
                                    /* Backward compatible: use first match */
                                    table_name = found_ed->table_name;
                                    schema_name = found_ed->schema_name[0] != '\0' ? found_ed->schema_name : schema_name;
                                    column_name = found_ed->column_name;
                                }
                            }
                        }

                        /* Get actual DEK name from cache */
                        if (num_fields >= 3)
                        {
                            Node *first_field = (Node *) linitial(col_ref->fields);
                            schema_name = strVal((Value *)first_field);
                        }

                        /* Use tecc_get_column_dek_name to get actual DEK name from cache */
                        actual_dek_name = tecc_get_column_dek_name(
                            statement_data, schema_name, table_name, column_name);
                        if (actual_dek_name != NULL)
                        {
                            dek_name = actual_dek_name;
                            /* Get KEK name from cache */
                            kek_name = where_get_column_kek_name(statement_data, schema_name, table_name, column_name);
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                               "Using DEK name from cache: %s for column %s.%s",
                                               dek_name, table_name ? table_name : "unknown", column_name);
                        }
                        else
                        {
                            /* ReqID:TECC4.0 - Column not encrypted: skip encryption
                             * and return success (query proceeds unmodified).
                             * This handles non-encrypted columns in WHERE clauses
                             * without failing the entire query. */
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                               "Column %s.%s not encrypted, skipping WHERE encryption",
                                               table_name ? table_name : "unknown",
                                               column_name ? column_name : "unknown");
                            return TECC_WHERE_PROCESSOR_SUCCESS;
                        }
                    }
                }
            }
            else if (nodeTag(col_ref_node) == FE_T_FuncCall)
            {
                /* ReqID:TECC4.0 - Extract column info from FuncCall args */
                FuncCall *fc = (FuncCall *) col_ref_node;
                ListCell *alc;
                bool found_in_fc = false;
                foreach(alc, fc->args)
                {
                    Node *arg = (Node *) alc->data.ptr_value;
                    if (arg != NULL && nodeTag(arg) == FE_T_ColumnRef)
                    {
                        struct ColumnRef *cr = (struct ColumnRef *) arg;
                        if (cr->fields != NIL)
                        {
                            int nf = list_length(cr->fields);
                            Node *f = (Node *) list_nth(cr->fields, nf - 1);
                            const char *col_table_name = NULL;
                            column_name = strVal((Value *) f);

                            /* ReqID:TECC4.0 - Extract table name from ColumnRef if available */
                            if (nf >= 2)
                            {
                                Node *tf = (Node *) list_nth(cr->fields, nf - 2);
                                col_table_name = strVal((Value *) tf);
                            }

                            if (column_name != NULL &&
                                where_is_encrypted_column_full(col_table_name, column_name, statement_data))
                            {
                                /* Get table name from encrypted_columns list */
                                ListCell *elc;
                                foreach(elc, statement_data->encrypted_columns)
                                {
                                    TeccProcessorColumnRef *ed = (TeccProcessorColumnRef *) lfirst(elc);
                                    if (ed == NULL || ed->column_name[0] == '\0')
                                        continue;
                                    if (strcmp(ed->column_name, column_name) != 0)
                                        continue;
                                    if (col_table_name != NULL)
                                    {
                                        if (strcmp(ed->table_alias, col_table_name) != 0 &&
                                            strcmp(ed->table_name, col_table_name) != 0)
                                            continue;
                                    }
                                    table_name = ed->table_name;
                                    schema_name = ed->schema_name[0] != '\0' ?
                                        ed->schema_name : schema_name;
                                    found_in_fc = true;
                                    break;
                                }
                                if (found_in_fc)
                                    break;
                            }
                        }
                    }
                }
                if (found_in_fc)
                {
                    actual_dek_name = tecc_get_column_dek_name(
                        statement_data, schema_name, table_name, column_name);
                    /* ReqID:TECC4.0 - Fallback: if tecc_get_column_dek_name fails
                     * (e.g. encrypted_columns entry from FROM clause lacks dek_name),
                     * do a direct cache lookup like stmt_process_funcall_constants. */
                    if (actual_dek_name == NULL && statement_data->conn != NULL)
                    {
                        TeccState *s = tecc_get_state(statement_data->conn);
                        const char *db = PQdb(statement_data->conn);
                        if (s != NULL && db != NULL && table_name != NULL)
                        {
                            /* 优先用已解析的 schema_name（限定表名如 sch1.tb1），
                             * 未限定才回退 current_schema */
                            const char *schema = schema_name;
                            if (schema == NULL || schema[0] == '\0')
                                schema = tecc_cache_get_guc_param(s, "current_schema");
                            if (schema != NULL)
                            {
                                TeccColumnCacheItem *enc_cache =
                                    tecc_cache_get_encrypted_column_by_name(
                                        s, db, schema, table_name, column_name);
                                if (enc_cache != NULL &&
                                    enc_cache->dek_name != NULL &&
                                    enc_cache->dek_name[0] != '\0')
                                {
                                    actual_dek_name = enc_cache->dek_name;
                                }
                            }
                        }
                    }
                    if (actual_dek_name != NULL)
                    {
                        dek_name = actual_dek_name;
                        kek_name = where_get_column_kek_name(statement_data, schema_name,
                                                              table_name, column_name);
                    }
                }
                if (!found_in_fc || dek_name == NULL)
                {
                    /* ReqID:TECC4.0 - Column not encrypted or not found:
                     * skip encryption and return success (query proceeds unmodified) */
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                       "FuncCall: column %s.%s not encrypted, skipping WHERE encryption",
                                       table_name ? table_name : "unknown",
                                       column_name ? column_name : "unknown");
                    return TECC_WHERE_PROCESSOR_SUCCESS;
                }
            }
            else if (nodeTag(col_ref_node) == FE_T_CaseExpr)
            {
                /* ReqID:TECC4.0 - Extract ColumnRef from CaseExpr tree for DEK lookup */
                struct ColumnRef *found_cr;
                const char *extracted_table_name;
                ListCell *elc;
                TeccProcessorColumnRef *found_ed;
                int ed_match_cnt;

                found_cr = extract_first_column_ref_from_node(col_ref_node);
                extracted_table_name = NULL;
                if (found_cr != NULL && found_cr->fields != NIL)
                {
                    int nf;
                    Node *last_field;
                    nf = list_length(found_cr->fields);
                    last_field = (Node *) llast(found_cr->fields);
                    column_name = strVal((Value *)last_field);
                    if (nf >= 2)
                    {
                        Node *tf;
                        tf = (Node *) list_nth(found_cr->fields, nf - 2);
                        extracted_table_name = strVal((Value *)tf);
                    }
                    if (nf >= 3)
                    {
                        Node *sf;
                        sf = (Node *) linitial(found_cr->fields);
                        schema_name = strVal((Value *)sf);
                    }

                    /* Resolve real table name from encrypted_columns */
                    found_ed = NULL;
                    ed_match_cnt = 0;
                    if (statement_data->encrypted_columns != NIL && column_name != NULL)
                    {
                        foreach(elc, statement_data->encrypted_columns)
                        {
                            TeccProcessorColumnRef *ed;
                            ed = (TeccProcessorColumnRef *) lfirst(elc);
                            if (ed == NULL || ed->column_name[0] == '\0')
                                continue;
                            if (strcmp(ed->column_name, column_name) != 0)
                                continue;
                            if (extracted_table_name != NULL)
                            {
                                if (strcmp(ed->table_alias, extracted_table_name) == 0 ||
                                    strcmp(ed->table_name, extracted_table_name) == 0)
                                {
                                    found_ed = ed;
                                    ed_match_cnt = 1;
                                    break;
                                }
                            }
                            else
                            {
                                found_ed = ed;
                                ed_match_cnt++;
                            }
                        }
                    }
                    if (found_ed != NULL && ed_match_cnt == 1)
                    {
                        table_name = found_ed->table_name;
                        if (found_ed->schema_name[0] != '\0')
                            schema_name = found_ed->schema_name;
                    }
                    else if (ed_match_cnt > 1 && !statement_data->multi_table_context && found_ed != NULL)
                    {
                        table_name = found_ed->table_name;
                        if (found_ed->schema_name[0] != '\0')
                            schema_name = found_ed->schema_name;
                    }

                    if (table_name == NULL && extracted_table_name != NULL)
                        table_name = extracted_table_name;

                    /* Look up DEK from cache */
                    if (table_name != NULL && column_name != NULL)
                    {
                        actual_dek_name = tecc_get_column_dek_name(
                            statement_data, schema_name, table_name, column_name);
                        if (actual_dek_name != NULL)
                        {
                            dek_name = actual_dek_name;
                            kek_name = where_get_column_kek_name(statement_data, schema_name,
                                                                  table_name, column_name);
                        }
                    }
                }
                if (dek_name == NULL)
                {
                    /* ReqID:TECC4.0 - Column not encrypted or not found:
                     * skip encryption and return success (query proceeds unmodified) */
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                       "CaseExpr: column %s.%s not encrypted, skipping WHERE encryption",
                                       table_name ? table_name : "unknown",
                                       column_name ? column_name : "unknown");
                    return TECC_WHERE_PROCESSOR_SUCCESS;
                }
            }
            /* ReqID:TECC4.0 - CollateClause wraps a column ref */
            else if (nodeTag(col_ref_node) == FE_T_CollateClause)
            {
                struct ColumnRef *found_cr;
                const char *extracted_table_name;
                ListCell *elc;
                TeccProcessorColumnRef *found_ed;
                int ed_match_cnt;

                found_cr = extract_first_column_ref_from_node(col_ref_node);
                extracted_table_name = NULL;
                if (found_cr != NULL && found_cr->fields != NIL)
                {
                    int nf;
                    Node *last_field;
                    nf = list_length(found_cr->fields);
                    last_field = (Node *) llast(found_cr->fields);
                    column_name = strVal((Value *)last_field);
                    if (nf >= 2)
                    {
                        Node *tf;
                        tf = (Node *) list_nth(found_cr->fields, nf - 2);
                        extracted_table_name = strVal((Value *)tf);
                    }
                    if (nf >= 3)
                    {
                        Node *sf;
                        sf = (Node *) linitial(found_cr->fields);
                        schema_name = strVal((Value *)sf);
                    }

                    /* Resolve real table name from encrypted_columns */
                    found_ed = NULL;
                    ed_match_cnt = 0;
                    if (statement_data->encrypted_columns != NIL && column_name != NULL)
                    {
                        foreach(elc, statement_data->encrypted_columns)
                        {
                            TeccProcessorColumnRef *ed;
                            ed = (TeccProcessorColumnRef *) lfirst(elc);
                            if (ed == NULL || ed->column_name[0] == '\0')
                                continue;
                            if (strcmp(ed->column_name, column_name) != 0)
                                continue;
                            if (extracted_table_name != NULL)
                            {
                                if (strcmp(ed->table_alias, extracted_table_name) == 0 ||
                                    strcmp(ed->table_name, extracted_table_name) == 0)
                                {
                                    found_ed = ed;
                                    ed_match_cnt = 1;
                                    break;
                                }
                            }
                            else
                            {
                                found_ed = ed;
                                ed_match_cnt++;
                            }
                        }
                    }
                    if (found_ed != NULL && ed_match_cnt == 1)
                    {
                        table_name = found_ed->table_name;
                        if (found_ed->schema_name[0] != '\0')
                            schema_name = found_ed->schema_name;
                    }
                    else if (ed_match_cnt > 1 && !statement_data->multi_table_context && found_ed != NULL)
                    {
                        table_name = found_ed->table_name;
                        if (found_ed->schema_name[0] != '\0')
                            schema_name = found_ed->schema_name;
                    }

                    if (table_name == NULL && extracted_table_name != NULL)
                        table_name = extracted_table_name;

                    if (table_name != NULL && column_name != NULL)
                    {
                        actual_dek_name = tecc_get_column_dek_name(
                            statement_data, schema_name, table_name, column_name);
                        if (actual_dek_name != NULL)
                        {
                            dek_name = actual_dek_name;
                            kek_name = where_get_column_kek_name(statement_data, schema_name,
                                                                  table_name, column_name);
                        }
                    }
                }
                if (dek_name == NULL)
                {
                    /* ReqID:TECC4.0 - Column not encrypted or not found:
                     * skip encryption and return success (query proceeds unmodified) */
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                       "CollateClause: column %s.%s not encrypted, skipping WHERE encryption",
                                       table_name ? table_name : "unknown",
                                       column_name ? column_name : "unknown");
                    return TECC_WHERE_PROCESSOR_SUCCESS;
                }
            }
            else
            {
                /* ReqID:TECC4.0 - Unknown node type: skip encryption and return
                 * success (query proceeds unmodified) */
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "ColumnRef/FuncCall node not available, skipping WHERE encryption");
                return TECC_WHERE_PROCESSOR_SUCCESS;
            }
            }

            /* ReqID:TECC4.0 - Abort if key names could not be resolved.
             * Without valid key names, encryption cannot proceed and the
             * SQL must not be sent to the backend. */
            if (dek_name == NULL || kek_name == NULL)
            {
                tecc_processor_debug(TECC_LOG_ERROR,
                                   "Cannot encrypt WHERE clause value: "
                                   "DEK/KEK names not resolved for column %s.%s",
                                   table_name ? table_name : "?",
                                   column_name ? column_name : "?");
                return TECC_WHERE_PROCESSOR_ERROR;
            }

            encrypt_result = NULL;

            /* ReqID:TECC4.0 - HE 标量运算常量加密：+ / - 用 EC-ElGamal 加密
             * 常量（EG_ADD 段），* 用 FF ElGamal 加密常量（EG_MUL 段），
             * 生成完整 intN_enc 密文改写为 `enc + 'hex'::intN_enc`，
             * 服务端走全密文同态操作符（he_add_generic / he_sub_generic /
             * he_mul_generic），常量明文不出前端。 */
            if (statement_data->operator_type == TECC_OP_ADD ||
                statement_data->operator_type == TECC_OP_SUB)
            {
                encrypt_result = tecc_unified_encrypt_he_constant(
                    statement_data->conn,
                    const_value,
                    table_name,
                    column_name,
                    schema_name,
                    true    /* use_he_add: EC-ElGamal EG_ADD 段 */
                );
            }
            else if (statement_data->operator_type == TECC_OP_MUL)
            {
                encrypt_result = tecc_unified_encrypt_he_constant(
                    statement_data->conn,
                    const_value,
                    table_name,
                    column_name,
                    schema_name,
                    false   /* use_he_add=false: FF ElGamal EG_MUL 段 */
                );
            }
            /* Determine which encryption function to call based on operator type */
            else if (statement_data->operator_type == TECC_OP_LIKE ||
                statement_data->operator_type == TECC_OP_NOT_LIKE ||
                statement_data->operator_type == TECC_OP_ILIKE ||
                statement_data->operator_type == TECC_OP_NOT_ILIKE)
            {
                /* Use operator-specific encryption for WHERE */
                

                encrypt_result = tecc_unified_encrypt_for_where(
                    statement_data->conn,
                    const_value,
                    table_name,
                    column_name,
                    schema_name,
                    kek_name,
                    dek_name,
                    statement_data->operator_type,
                    statement_data->like_escape_char,
                    NULL
                );
            }
            else
            {
                /* Use operator-specific encryption for WHERE */


                encrypt_result = tecc_unified_encrypt_for_where(
                    statement_data->conn,
                    const_value,
                    table_name,
                    column_name,
                    schema_name,
                    kek_name,
                    dek_name,
                    statement_data->operator_type,
                    0,
                    NULL
                );
            }

            /* Check encryption result */
            if (encrypt_result != NULL)
            {
                if (encrypt_result->error_code == TECC_CRYPTO_SUCCESS)
                {
                    

                    /* Store encrypted value in statement_data for use in rewrite phase */
                    raw_value = tecc_raw_value_create_from_string(const_value, strlen(const_value));
                    if (raw_value != NULL)
                    {
                        /* Use Const->location/data_size from parser if available.
                         * ReqID:TECC4.0 - Also handles UMINUS+Const via const_loc/const_dsize. */
                        if (const_dsize > 0)
                        {
                            raw_value->location = const_loc;
                            raw_value->data_size = const_dsize;
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                               "WHERE raw_value from Const: loc=%zu, data_size=%zu",
                                               raw_value->location, raw_value->data_size);
                        }
                        else if (const_expr != NULL && const_expr->location >= 0 && const_expr->data_size > 0)
                        {
                            raw_value->location = (size_t)const_expr->location;
                            raw_value->data_size = (size_t)const_expr->data_size;
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                               "WHERE raw_value from Const: loc=%zu, data_size=%zu",
                                               raw_value->location, raw_value->data_size);
                        }

                        /* ReqID:TECC4.0 - Set column_type for explicit ::type_name cast
                         * in rewritten SQL. Needed for SubLink UNION type resolution.
                         * Look up the column's actual encryption type from cache to get
                         * the correct type (varchar_enc/bpchar_enc) instead of deriving
                         * it from the constant type (which is always text_enc for strings). */
                        {
                            const char *enc_type_name = NULL;

                            /* Try to get column's encryption type from metadata cache */
                            if (statement_data->conn != NULL
                                && table_name != NULL
                                && column_name != NULL)
                            {
                                TeccState *s = tecc_get_state(statement_data->conn);
                                const char *db = PQdb(statement_data->conn);
                                if (s != NULL && db != NULL)
                                {
                                    const char *schema = schema_name;
                                    if (schema == NULL || schema[0] == '\0')
                                        schema = tecc_cache_get_guc_param(
                                            s, "current_schema");
                                    if (schema != NULL)
                                    {
                                        TeccColumnCacheItem *col_cache =
                                            tecc_cache_get_encrypted_column_by_name(
                                                s, db, schema, table_name, column_name);
                                        if (col_cache != NULL)
                                            enc_type_name = get_enc_type_name_from_base_oid(
                                                col_cache->original_type_oid);
                                    }
                                }
                            }
                            /* Cache miss: leave enc_type_name=NULL.
                             * Do NOT fall back to constant's type — that would
                             * produce wrong results (e.g. text_enc for varchar_enc). */
                            tecc_raw_value_set_column_info(raw_value, column_name,
                                                           enc_type_name, InvalidOid);
                        }
                        if (table_name != NULL)
                        {
                            tecc_raw_value_set_table_info(raw_value, schema_name, table_name);
                        }

                        /* Store encrypted value */
                        if (raw_value->processed.encrypted_value != NULL)
                        {
                            pg_free(raw_value->processed.encrypted_value);
                        }
                        raw_value->processed.encrypted_value = tecc_processor_strdup(encrypt_result->hex_string);
                        raw_value->state = TECC_RAW_VALUE_STATE_PROCESSED;
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                           "WHERE value processed: column=%s, loc=%zu, data_size=%zu, table=%s",
                                           column_name ? column_name : "NULL",
                                           raw_value->location, raw_value->data_size,
                                           table_name ? table_name : "NULL");

                        /* Add WHERE raw_value to raw_values_list for all statements.
                         * Skip duplicates (same location/data_size) — rewrite phase
                         * also adds entries. ReqID:TECC4.0 */
                        if (statement_data->raw_values_list == NULL)
                            statement_data->raw_values_list = tecc_raw_values_list_create(16);
                        if (statement_data->raw_values_list != NULL) {
                            bool dup = false;
                            size_t vi;
                            for (vi = 0; vi < statement_data->raw_values_list->num_values; vi++) {
                                TeccRawValue *e = statement_data->raw_values_list->values[vi];
                                if (e && e->location == raw_value->location && e->data_size == raw_value->data_size)
                                { dup = true; break; }
                            }
                            if (!dup) {
                                if (tecc_raw_values_list_add(statement_data->raw_values_list, raw_value) < 0)
                                    tecc_raw_value_destroy(raw_value);
                            } else {
                                tecc_raw_value_destroy(raw_value);
                            }
                        } else {
                            tecc_raw_value_destroy(raw_value);
                        }
                        raw_value = NULL;
                    }
                    else
                    {
                        tecc_processor_debug(TECC_LOG_WARNING,
                                           "Failed to create raw value for encrypted constant");
                    }
                }
                else
                {
                    tecc_processor_debug(TECC_LOG_ERROR,
                                       "Encryption failed: %s",
                                       encrypt_result->error_msg ? encrypt_result->error_msg : "unknown");
                }

                /* Free encryption result */
                tecc_unified_free_encrypt_result(encrypt_result);
            }
            else
            {
                tecc_processor_debug(TECC_LOG_ERROR,
                                   "Encryption result is NULL");
            }

            /* Mark that encryption is needed */
            statement_data->where_needs_rewrite = 1;
            statement_data->needs_rewrite = 1;
            /* End - ReqID:TECC4.0 - 调用加密调度器 */

            /* Free the allocated memory for constant value */
            pg_free(const_value);
            const_value = NULL;
        }
    }

    /* Free allocated DEK name memory */
    if (actual_dek_name != NULL)
    {
        pg_free(actual_dek_name);
        actual_dek_name = NULL;
    }

    return TECC_WHERE_PROCESSOR_SUCCESS;
}

/*
 * Extract column references from WHERE clause
 */
List *
tecc_where_clause_processor_extract_columns(Node *where_clause)
{
    List *columns;
    TeccExprPartsList *expr_parts = NULL;
    bool expand_result;

    if (where_clause == NULL)
        return NIL;

    columns = NIL;

    /* Use expression processor to expand the WHERE clause */
    expand_result = tecc_expand_condition_expr(where_clause, &expr_parts);
    if (!expand_result || expr_parts == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to expand WHERE clause expression");
        return columns;
    }

    /* Extract column references from expression parts */
    {
        size_t i;
        for (i = 0; i < expr_parts->count; i++)
        {
            TeccExprPart *part = expr_parts->parts[i];
            if (part->type == FE_T_ColumnRef && part->data.column_ref != NULL)
            {
                TeccColumnRefData *col_data = part->data.column_ref;
                columns = lappend(columns, makeStringConst(col_data->column_name, 0, 0));
            }
        }
    }

    /* Clean up expression parts */
    if (expr_parts != NULL)
    {
        tecc_expr_parts_list_destroy(expr_parts);
    }

    return columns;
}

/*
 * Check if WHERE clause contains encrypted columns
 */
bool
tecc_where_clause_processor_has_encrypted_columns(Node *where_clause,
                                                 TeccStatementData *statement_data)
{
    List *columns;

    if (where_clause == NULL || statement_data == NULL)
        return false;

    /* Extract column references */
    columns = tecc_where_clause_processor_extract_columns(where_clause);
    if (columns == NIL)
        return false;

    /* Check if any column is encrypted */
    {
        ListCell *lc;
        foreach(lc, columns)
        {
            Value *value_node = (Value *)lfirst(lc);
            char *column_name = strVal(value_node);
            if (column_name && where_is_encrypted_column(column_name, statement_data))
                return true;
        }
    }

    return false;
}

/* Begin - ReqID:TECC4.0 - CASE 分支常量按密态列算法模式加密 */

/*
 * where_case_const_type_compatible - 判断常量类型能否按密态列的原始类型加密。
 *
 * 密态列原始类型（original_type_oid）与常量 consttype 兼容时返回 true：
 *   - 整数密态类型（int2/int4/int8_enc）接受整型常量（int2/4/8）；
 *   - 浮点/数值密态类型（float4/float8/numeric_enc）接受整型、浮点、数值常量；
 *   - 字符密态类型（varchar/bpchar/text_enc）接受字符串常量（text/varchar/bpchar/cstring）；
 *   - 其余组合不兼容（如 int2_enc 列配字符串常量），前端无法确定加密方式，报错。
 */
static bool
where_case_const_type_compatible(Oid enc_original_type_oid, Oid consttype)
{
    switch (enc_original_type_oid)
    {
        case INT2OID:
        case INT4OID:
        case INT8OID:
            return (consttype == INT2OID || consttype == INT4OID ||
                    consttype == INT8OID);

        case FLOAT4OID:
        case FLOAT8OID:
        case NUMERICOID:
            return (consttype == INT2OID || consttype == INT4OID ||
                    consttype == INT8OID || consttype == FLOAT4OID ||
                    consttype == FLOAT8OID || consttype == NUMERICOID);

        case VARCHAROID:
        case BPCHAROID:
        case TEXTOID:
            return (consttype == TEXTOID || consttype == VARCHAROID ||
                    consttype == BPCHAROID || consttype == CSTRINGOID);

        default:
            return false;
    }
}

/*
 * where_process_case_branch_const - 加密 CASE 分支中的普通常量。
 *
 * 当 CASE 的某个分支返回密态列、兄弟分支返回普通常量时，将常量按密态列的
 * 完整算法模式（RND + 列配置的全部对称算法段，与 INSERT 路径一致）加密，
 * 使服务端返回的常量密文与密态列存储密文格式一致，客户端可统一解密。
 *
 * 与 WHERE 比较常量加密（tecc_unified_encrypt_for_where，仅加密单算法段）
 * 不同，分支常量作为结果返回，必须与列存储格式一致，因此走
 * tecc_unified_encrypt_string_with_schema（INSERT 全算法路径）。
 *
 * 返回 true 表示处理成功（常量已加密并登记改写）；返回 false 表示处理失败
 * （类型不兼容或加密失败，错误信息已设置到 statement_data）。
 */
static bool
where_process_case_branch_const(Node *encrypted_colref, Node *const_node,
                                TeccStatementData *statement_data)
{
    struct ColumnRef *col_ref;
    char *col_name = NULL;
    const char *schema_name = NULL;
    const char *table_name = NULL;
    Const *const_expr;
    char *const_value = NULL;
    TeccUnifiedEncryptResult *encrypt_result = NULL;
    TeccState *state = NULL;
    TeccColumnCacheItem *cache_item = NULL;
    const char *db_name;
    const char *kek_name = NULL;
    const char *dek_name = NULL;
    TeccRawValue *raw_value = NULL;
    Oid enc_oid;
    int replace_result;

    if (encrypted_colref == NULL || const_node == NULL || statement_data == NULL)
        return false;

    /* 仅处理 ColumnRef（密态列）与 Const（普通常量） */
    if (nodeTag(encrypted_colref) != FE_T_ColumnRef ||
        nodeTag(const_node) != FE_T_Const)
        return false;

    col_ref = (struct ColumnRef *) encrypted_colref;
    const_expr = (Const *) const_node;

    /* 提取列名（最后一个字段） */
    if (col_ref->fields != NIL)
    {
        int nf = list_length(col_ref->fields);
        Node *f = (Node *) list_nth(col_ref->fields, nf - 1);
        col_name = strVal((Value *) f);
    }
    if (col_name == NULL)
        return false;

    /* 解析 schema/table：优先取 ColumnRef 中的限定名，否则从
     * statement_data->encrypted_columns 列表解析（与 WHERE 处理一致） */
    if (list_length(col_ref->fields) >= 2)
    {
        Node *t = (Node *) list_nth(col_ref->fields, list_length(col_ref->fields) - 2);
        table_name = strVal((Value *) t);
    }
    if (list_length(col_ref->fields) >= 3)
    {
        Node *s = (Node *) list_nth(col_ref->fields, list_length(col_ref->fields) - 3);
        schema_name = strVal((Value *) s);
    }

    /* 从 statement_data->encrypted_columns 解析真实表名/模式名 */
    if (statement_data->encrypted_columns != NIL)
    {
        ListCell *lc;
        TeccColumnRefData *found_ed = NULL;
        int match_cnt = 0;

        foreach(lc, statement_data->encrypted_columns)
        {
            TeccColumnRefData *ed = (TeccColumnRefData *) lfirst(lc);
            if (ed == NULL || ed->column_name[0] == '\0')
                continue;
            /* Bug 9323 - 重命名后的输出列按 column_alias 匹配 */
            if (strcmp(ed->column_name, col_name) != 0 &&
                (ed->column_alias[0] == '\0' ||
                 strcmp(ed->column_alias, col_name) != 0))
                continue;
            if (table_name != NULL)
            {
                if (strcmp(ed->table_alias, table_name) == 0 ||
                    strcmp(ed->table_name, table_name) == 0)
                {
                    found_ed = ed;
                    match_cnt = 1;
                    break;
                }
            }
            else
            {
                found_ed = ed;
                match_cnt++;
            }
        }

        if (found_ed != NULL && match_cnt == 1)
        {
            table_name = found_ed->table_name;
            /* Bug 9323 - 命中可能来自 column_alias，回填底层列名 */
            col_name = found_ed->column_name;
            schema_name = (found_ed->schema_name[0] != '\0') ?
                found_ed->schema_name : "public";
        }
        else if (found_ed != NULL)
        {
            /* 多表歧义时回退：使用第一个匹配 */
            table_name = found_ed->table_name;
            col_name = found_ed->column_name;
            schema_name = (found_ed->schema_name[0] != '\0') ?
                found_ed->schema_name : "public";
        }
    }

    if (table_name == NULL || schema_name == NULL)
    {
        tecc_processor_debug(TECC_LOG_WARNING,
                           "CASE branch: cannot resolve table for column %s",
                           col_name ? col_name : "?");
        return false;
    }

    /* 查列缓存获取原始类型与密钥信息 */
    state = tecc_get_state(statement_data->conn);
    if (state == NULL)
        return false;

    db_name = PQdb(statement_data->conn);
    if (db_name == NULL || db_name[0] == '\0')
        db_name = "postgres";

    cache_item = tecc_cache_get_encrypted_column_by_name(
        state, db_name, schema_name, table_name, col_name);
    if (cache_item == NULL)
    {
        tecc_processor_debug(TECC_LOG_WARNING,
                           "CASE branch: column %s.%s not found in cache",
                           table_name, col_name);
        return false;
    }

    /* 类型兼容校验：常量类型须能按密态列原始类型加密。
     * ReqID:TECC4.0 - 需求 4.4.1.4 固定错误格式（F 模板）。
     * 注意：浮点字面量在前端被原样保留为 TEXTOID 字符串常量以保精度
     * （见 frontend_parser/tecc_grammar.y 的 FLOAT 产生式），其 consttype
     * 不能直接参与判定，否则 CASE WHEN c_f4 > 10 THEN c_f4 ELSE 0.5 END
     * 会被误判为"分支常量类型无法按密态列加密"。带 text_numeric_literal
     * 标记的常量按其数值语义处理。 */
    if (!where_case_const_type_compatible(
            cache_item->original_type_oid,
            (const_expr->consttype == TEXTOID &&
             const_expr->text_numeric_literal)
                ? NUMERICOID : const_expr->consttype))
    {
        {
            char reason_buf[512];

            snprintf(reason_buf, sizeof(reason_buf),
                     stmt_f_reason_text(STMT_F_REASON_CASE_BRANCH), col_name);
            tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                stmt_f_template(), "case", reason_buf);
        }
        return false;
    }

    /* 提取常量值（用于 raw_value 原始值登记） */
    const_value = where_extract_const_value(const_expr, statement_data);
    if (const_value == NULL)
        return false;

    /* 获取 KEK/DEK 名称 */
    kek_name = where_get_column_kek_name(statement_data, schema_name,
                                         table_name, col_name);
    dek_name = tecc_get_column_dek_name(statement_data, schema_name,
                                        table_name, col_name);
    if (kek_name == NULL || dek_name == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "CASE branch: cannot resolve KEK/DEK for %s.%s",
                           table_name, col_name);
        pg_free(const_value);
        return false;
    }

    /* 按 INSERT 全算法路径加密（RND + 列配置的全部对称算法段） */
    encrypt_result = tecc_unified_encrypt_string_with_schema(
        statement_data->conn,
        schema_name,
        const_value,
        table_name,
        col_name,
        kek_name,
        dek_name,
        NULL);
    if (encrypt_result == NULL ||
        encrypt_result->error_code != TECC_CRYPTO_SUCCESS ||
        encrypt_result->hex_string == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "CASE branch: failed to encrypt constant for %s.%s",
                           table_name, col_name);
        if (encrypt_result != NULL)
            tecc_unified_free_encrypt_result(encrypt_result);
        pg_free(const_value);
        return false;
    }

    /* 创建 raw_value 登记改写：original 存明文，processed 存密文，
     * location/data_size 用于 SQL 文本替换 */
    raw_value = tecc_raw_value_create_from_string(const_value, strlen(const_value));
    pg_free(const_value);
    if (raw_value == NULL)
    {
        tecc_unified_free_encrypt_result(encrypt_result);
        return false;
    }

    if (raw_value->processed.encrypted_value != NULL)
        pg_free(raw_value->processed.encrypted_value);
    raw_value->processed.encrypted_value =
        tecc_processor_strdup(encrypt_result->hex_string);
    raw_value->state = TECC_RAW_VALUE_STATE_PROCESSED;
    tecc_unified_free_encrypt_result(encrypt_result);
    if (raw_value->processed.encrypted_value == NULL)
    {
        tecc_raw_value_destroy(raw_value);
        return false;
    }

    if (const_expr->location >= 0 && const_expr->data_size > 0)
    {
        raw_value->location = (size_t) const_expr->location;
        raw_value->data_size = (size_t) const_expr->data_size;
    }
    tecc_raw_value_set_column_info(raw_value, col_name,
                                   get_enc_type_name_from_base_oid(
                                       cache_item->original_type_oid),
                                   InvalidOid);
    tecc_raw_value_set_table_info(raw_value, schema_name, table_name);

    /* 替换 parse tree 中的常量
     * ReqID:TECC4.0 - 浮点字面量（TEXTOID 承载）须按密态列基类型映射 enc
     * 类型；若仍用 consttype 会被映射成 text_enc，与列类型不匹配。 */
    enc_oid = where_get_encrypted_type_oid(
        (const_expr->consttype == TEXTOID && const_expr->text_numeric_literal)
            ? cache_item->original_type_oid : const_expr->consttype);
    replace_result = where_replace_const_value(&const_node, raw_value->processed.encrypted_value,
                                               statement_data, enc_oid);
    if (replace_result != 0)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "CASE branch: failed to replace constant value");
        tecc_raw_value_destroy(raw_value);
        return false;
    }

    /* 登记到 raw_values_list 供改写阶段替换 SQL 文本 */
    if (statement_data->raw_values_list == NULL)
        statement_data->raw_values_list = tecc_raw_values_list_create(16);
    if (statement_data->raw_values_list != NULL)
    {
        if (tecc_raw_values_list_add(statement_data->raw_values_list, raw_value) < 0)
            tecc_raw_value_destroy(raw_value);
        else
            raw_value = NULL;
    }
    else
    {
        tecc_raw_value_destroy(raw_value);
        return false;
    }

    statement_data->needs_rewrite = 1;
    return true;
}

/* End - ReqID:TECC4.0 */

/*
 * Post-visit: second-pass encryption for CaseExpr result branches.
 *
 * After the walker has recursed into all CaseExpr children (and the
 * ColumnRef pre_visit has marked encrypted columns), inspect the
 * direct result branches of the CaseExpr.  If any branch result is
 * an encrypted ColumnRef, encrypt plain Const values in sibling
 * branches so the backend can determine a unified CASE result type.
 */
static bool
where_process_case_post(struct CaseExpr *ce, TeccStatementData *statement_data)
{
    Node *encrypted_colref = NULL;
    int   branch_count = 0;
    ListCell *clc;

    if (ce == NULL)
        return true;

    /* ReqID:TECC4.0 - 需求 4.4.1.4：CASE 参数组合统一校验（F 模板）。
     * 1) switch 形式（CASE 表达式 WHEN 常量）：表达式引用加密列时不支持
     *    （常量未按列加密，服务端无匹配操作符）；
     * 2) 分支/switch 表达式中出现裸 ::密态类型 cast 普通常量：服务端无
     *    明文→密态 cast 行，必报错，统一由前端拦截。 */
    if (ce->arg != NULL)
    {
        struct ColumnRef *cr = extract_first_column_ref_from_node(ce->arg);
        bool arg_has_enc_col = false;

        if (cr != NULL && cr->fields != NIL)
        {
            int nf = list_length(cr->fields);
            char *cn = strVal((Value *) list_nth(cr->fields, nf - 1));

            if (cn != NULL && where_is_encrypted_column(cn, statement_data))
                arg_has_enc_col = true;
        }
        if (arg_has_enc_col)
        {
            tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                stmt_f_template(), "case",
                stmt_f_reason_text(STMT_F_REASON_CASE_SWITCH));
            statement_data->where_processor_result = TECC_WHERE_PROCESSOR_ERROR;
            return false;
        }
        if (stmt_node_has_bare_enc_cast((Node *) ce->arg, statement_data))
        {
            tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                stmt_f_template(), "case",
                stmt_f_reason_text(STMT_F_REASON_BARE_CAST));
            statement_data->where_processor_result = TECC_WHERE_PROCESSOR_ERROR;
            return false;
        }
    }
    if (ce->args != NIL)
    {
        foreach(clc, ce->args)
        {
            struct CaseWhen *cw = (struct CaseWhen *) lfirst(clc);

            if (cw != NULL && cw->result != NULL &&
                stmt_node_has_bare_enc_cast((Node *) cw->result, statement_data))
            {
                tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
                    stmt_f_template(), "case",
                    stmt_f_reason_text(STMT_F_REASON_BARE_CAST));
                statement_data->where_processor_result = TECC_WHERE_PROCESSOR_ERROR;
                return false;
            }
        }
    }
    if (ce->defresult != NULL &&
        stmt_node_has_bare_enc_cast((Node *) ce->defresult, statement_data))
    {
        tecc_processor_set_error(statement_data, TECC_PROCESSOR_ERROR,
            stmt_f_template(), "case",
            stmt_f_reason_text(STMT_F_REASON_BARE_CAST));
        statement_data->where_processor_result = TECC_WHERE_PROCESSOR_ERROR;
        return false;
    }

    /* Find first encrypted ColumnRef in CaseWhen result branches.
     * ReqID:TECC4.0 - 递归提取分支中的列引用（支持嵌套 CASE 分支），
     * 使外层 CASE 能识别"分支包含加密列"并加密兄弟常量分支。 */
    if (ce->args != NIL)
    {
        foreach(clc, ce->args)
        {
            struct CaseWhen *cw = (struct CaseWhen *) lfirst(clc);
            if (cw != NULL && cw->result != NULL)
            {
                if (encrypted_colref == NULL)
                {
                    struct ColumnRef *cr;
                    char *col_name = NULL;

                    cr = extract_first_column_ref_from_node(cw->result);
                    if (cr != NULL && cr->fields != NIL)
                    {
                        int nf = list_length(cr->fields);
                        Node *f = (Node *) list_nth(cr->fields, nf - 1);
                        col_name = strVal((Value *) f);
                    }
                    if (col_name != NULL &&
                        where_is_encrypted_column(col_name, statement_data))
                        encrypted_colref = (Node *) cr;
                }
                branch_count++;
            }
        }
    }

    /* Also check ELSE (defresult) */
    if (ce->defresult != NULL)
    {
        if (encrypted_colref == NULL)
        {
            struct ColumnRef *cr;
            char *col_name = NULL;

            cr = extract_first_column_ref_from_node(ce->defresult);
            if (cr != NULL && cr->fields != NIL)
            {
                int nf = list_length(cr->fields);
                Node *f = (Node *) list_nth(cr->fields, nf - 1);
                col_name = strVal((Value *) f);
            }
            if (col_name != NULL &&
                where_is_encrypted_column(col_name, statement_data))
                encrypted_colref = (Node *) cr;
        }
        branch_count++;
    }

    /* Second pass: encrypt plain Const branches.
     * ReqID:TECC4.0 - 分支常量按密态列完整算法模式加密（INSERT 路径），
     * 使常量密文与列存储密文格式一致，服务端返回后客户端统一解密。 */
    if (encrypted_colref != NULL && branch_count > 1)
    {
        if (ce->args != NIL)
        {
            foreach(clc, ce->args)
            {
                struct CaseWhen *cw = (struct CaseWhen *) lfirst(clc);
                if (cw != NULL && cw->result != NULL &&
                    nodeTag(cw->result) == FE_T_Const)
                {
                    if (!where_process_case_branch_const(
                            encrypted_colref, cw->result, statement_data))
                    {
                        statement_data->where_processor_result =
                            TECC_WHERE_PROCESSOR_ERROR;
                        return false;
                    }
                }
            }
        }
        if (ce->defresult != NULL &&
            nodeTag(ce->defresult) == FE_T_Const)
        {
            if (!where_process_case_branch_const(
                    encrypted_colref, ce->defresult, statement_data))
            {
                statement_data->where_processor_result =
                    TECC_WHERE_PROCESSOR_ERROR;
                return false;
            }
        }
    }

    return true;
}

/*
 * Post-visit: encrypt subquery targetList constants when the outer
 * testexpr references an encrypted column (ANY/ALL sublinks).
 */
static bool
where_process_sublink_post(struct SubLink *sl, TeccStatementData *statement_data)
{
    bool testexpr_is_encrypted = false;
    char *encrypted_col_name = NULL;
    TeccWhereClauseProcessorResult result;

    if (sl == NULL)
        return true;

    /* Check if testexpr is an encrypted ColumnRef (direct inspection) */
    if (sl->testexpr != NULL &&
        nodeTag(sl->testexpr) == FE_T_ColumnRef)
    {
        struct ColumnRef *col_ref = (struct ColumnRef *) sl->testexpr;
        if (col_ref->fields != NIL)
        {
            int num_fields = list_length(col_ref->fields);
            Node *field = (Node *) list_nth(col_ref->fields, num_fields - 1);
            encrypted_col_name = strVal((Value *) field);
            if (encrypted_col_name != NULL)
                testexpr_is_encrypted = where_is_encrypted_column(
                    encrypted_col_name, statement_data);
        }
    }

    /* Encrypt subquery targetList constants */
    if (testexpr_is_encrypted && sl->operName != NIL &&
        sl->subLinkType != EXISTS_SUBLINK)
    {
        char *operator_name = NULL;
        Node *op_node = (Node *) llast(sl->operName);

        /* ReqID:TECC4.0 - Reject cross-DEK subquery comparisons
         * (e.g. v IN (SELECT ...) / v = ANY (SELECT ...) with different DEKs) */
        result = where_check_sublink_dek(sl, sl->testexpr, statement_data);
        if (result != TECC_WHERE_PROCESSOR_SUCCESS)
        {
            statement_data->where_processor_result = (int) result;
            return false;
        }

        if (op_node != NULL && nodeTag(op_node) == FE_T_String)
            operator_name = strVal((Value *) op_node);

        if (operator_name != NULL)
        {
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Encrypting SubLink target constants: op=%s, col=%s",
                               operator_name, encrypted_col_name);

            result = where_process_subquery_target_consts(
                sl->subselect, sl->testexpr,
                operator_name, statement_data);
            if (result != TECC_WHERE_PROCESSOR_SUCCESS)
                return false;
        }
    }

    return true;
}

/* ReqID:TECC4.0 - Post-visit callback for WHERE clause processing */
static bool
where_process_post_visit(Node *node, void *context)
{
    TeccStatementData *statement_data = (TeccStatementData *) context;

    switch (nodeTag(node))
    {
        case FE_T_A_Expr:
        {
            TeccWhereClauseProcessorResult result;
            result = where_process_a_expr((struct A_Expr *) node, statement_data);
            if (result != TECC_WHERE_PROCESSOR_SUCCESS)
                statement_data->where_processor_result = (int)result;
            return (result == TECC_WHERE_PROCESSOR_SUCCESS);
        }
        case FE_T_FuncCall:
            stmt_process_funcall_constants((FuncCall *) node, statement_data);
            /* ReqID:TECC4.0 - Validate aggregate functions against column
             * encryption algorithms (MIN/MAX need OPE; SUM/AVG/string_agg
             * rejected). This catches aggregates nested in expressions
             * (e.g. sum(x) + 1, HAVING min(x) > 5). */
            if (!stmt_validate_aggregate_funcall((FuncCall *) node, statement_data))
            {
                statement_data->where_processor_result =
                    TECC_WHERE_PROCESSOR_UNSUPPORTED_AGGREGATE;
                return false;
            }
            /* ReqID:TECC4.0 - 需求 4.4.1.4：参数组合统一校验
             * （coalesce/greatest/least，固定错误格式 F 模板）。 */
            if (!stmt_validate_scalar_arg_combos((FuncCall *) node, statement_data))
            {
                statement_data->where_processor_result =
                    TECC_WHERE_PROCESSOR_UNSUPPORTED_SCALAR;
                return false;
            }

            /* ReqID:TECC4.0 - Validate scalar functions against column
             * encryption algorithms (power rejected).  Catches scalar
             * functions nested in expressions. */
            if (!stmt_validate_scalar_funcall((FuncCall *) node, statement_data))
            {
                statement_data->where_processor_result =
                    TECC_WHERE_PROCESSOR_UNSUPPORTED_SCALAR;
                return false;
            }
            return true;
        case FE_T_CaseExpr:
            return where_process_case_post((struct CaseExpr *) node,
                                           statement_data);
        case FE_T_SubLink:
            return where_process_sublink_post((struct SubLink *) node,
                                              statement_data);
        default:
            return true;
    }
}

/* ReqID:TECC4.0 - Pre-visit callback: mark encrypted column references */
static bool
where_process_pre_visit(Node *node, void *context)
{
    if (nodeTag(node) == FE_T_ColumnRef)
    {
        where_process_column_ref((struct ColumnRef *) node,
                                 (TeccStatementData *) context);
    }
    /* ReqID:TECC4.0 - Load subquery FROM-table encrypted columns before
     * descending into the subquery, so correlated comparisons inside
     * (e.g. EXISTS ... WHERE e.v = a.v) can resolve the DEK of the
     * subquery-side column. */
    else if (nodeTag(node) == FE_T_SubLink)
    {
        where_load_sublink_tables((struct SubLink *) node,
                                  (TeccStatementData *) context);
    }
    return true;
}

/*
 * Process WHERE clause expression using unified walker.
 * Walk the expression tree with pre/post callbacks.
 */
static TeccWhereClauseProcessorResult
where_process_expr(Node *expr, TeccStatementData *statement_data)
{
    TeccExprWalkerCallbacks callbacks;

    if (expr == NULL)
        return TECC_WHERE_PROCESSOR_SUCCESS;

    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.pre_visit  = where_process_pre_visit;
    callbacks.post_visit = where_process_post_visit;

    /* Reset stored result before walk */
    statement_data->where_processor_result = TECC_WHERE_PROCESSOR_SUCCESS;

    if (!tecc_walk_expression_tree(expr, &callbacks, statement_data))
    {
        /* Walker failed — prefer the stored specific error code */
        if (statement_data->where_processor_result != TECC_WHERE_PROCESSOR_SUCCESS)
            return (TeccWhereClauseProcessorResult)statement_data->where_processor_result;
        return TECC_WHERE_PROCESSOR_ERROR;
    }

    return TECC_WHERE_PROCESSOR_SUCCESS;
}

static TeccWhereClauseProcessorResult
where_process_a_expr(struct A_Expr *expr, TeccStatementData *statement_data)
{
    TeccWhereClauseProcessorResult result;

    if (expr == NULL)
        return TECC_WHERE_PROCESSOR_SUCCESS;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing A_Expr with kind: %d", expr->kind);

    /* Process based on expression kind */
    switch (expr->kind)
    {
        case AEXPR_OP:
        case AEXPR_OP_ANY:
        case AEXPR_OP_ALL:
            /* ReqID:TECC4.0 - Unary operator: children already recursed by walker. */
            if (expr->lexpr == NULL || expr->rexpr == NULL)
                return TECC_WHERE_PROCESSOR_SUCCESS;

            /* Binary operator */
            if (expr->name != NIL)
            {
                Node *op_node = (Node *) llast(expr->name);
                char *operator_name = strVal((Value *)op_node);
                if (operator_name != NULL)
                {
                    /* ReqID:TECC4.0 - Children already recursed by walker.
                     * FuncCall constants encrypted by post_visit, then
                     * process_comparison runs here in post-order. */

                    /* ReqID:TECC4.0 - RowExpr element-wise comparison:
                     * (a,b) = ROW(1,2) → process each pair individually. */
                    if (nodeTag(expr->lexpr) == FE_T_RowExpr &&
                        nodeTag(expr->rexpr) == FE_T_RowExpr)
                    {
                        RowExpr *lrx = (RowExpr *)expr->lexpr;
                        RowExpr *rrx = (RowExpr *)expr->rexpr;
                        int llen, rlen;
                        TeccWhereClauseProcessorResult row_result = TECC_WHERE_PROCESSOR_SUCCESS;
                        ListCell *llc, *rlc;

                        llen = list_length(lrx->args);
                        rlen = list_length(rrx->args);
                        if (llen != rlen)
                        {
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                               "RowExpr element count mismatch: %d vs %d",
                                               llen, rlen);
                            return TECC_WHERE_PROCESSOR_SUCCESS;
                        }
                        for (llc = list_head(lrx->args), rlc = list_head(rrx->args);
                             llc != NULL && rlc != NULL;
                             llc = lnext(llc), rlc = lnext(rlc))
                        {
                            Node *lelem = (Node *)lfirst(llc);
                            Node *relem = (Node *)lfirst(rlc);
                            row_result = tecc_where_clause_processor_process_comparison(
                                lelem, relem, operator_name, statement_data);
                            if (row_result != TECC_WHERE_PROCESSOR_SUCCESS)
                                return row_result;
                        }
                        return TECC_WHERE_PROCESSOR_SUCCESS;
                    }

                    result = tecc_where_clause_processor_process_comparison(
                        expr->lexpr, expr->rexpr, operator_name, statement_data);
                    return result;
                }
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Operator name is not a string node");
            }
            break;

        /* Note: AND, OR, NOT operations are handled by BoolExpr, not A_Expr */
        /* case AEXPR_AND: case AEXPR_OR: case AEXPR_NOT: - these don't exist in PostgreSQL */
        /* Boolean operations are handled in where_process_bool_expr function */

        case AEXPR_LIKE:
            /* ReqID:TECC4.0 - LIKE expression: children already recursed by walker.
             * Use linitial(), not llast(): name list is {operator, escape} for
             * LIKE ... ESCAPE.  llast() would return the escape character
             * instead of the LIKE operator name. ReqID:TECC4.0 */
            if (expr->name != NIL)
            {
                Node *op_node = (Node *) linitial(expr->name);
                char *operator_name = strVal((Value *)op_node);
                if (operator_name != NULL)
                {
                    /* ReqID:TECC4.0 - Extract ESCAPE character from AEXPR_LIKE node.
                     * TECC frontend parser stores ESCAPE in expr->escape (dedicated field),
                     * NOT in expr->name[1]. expr->name only has 1 element (operator name). */
                    if (expr->escape != NULL && IsA(expr->escape, String))
                    {
                        char *esc_str = strVal((Value *)expr->escape);
                        if (esc_str != NULL && esc_str[0] != '\0')
                            statement_data->like_escape_char = esc_str[0];
                    }

                    result = tecc_where_clause_processor_process_comparison(
                        expr->lexpr, expr->rexpr, operator_name, statement_data);
                    return result;
                }
            }
            break;

        case AEXPR_SIMILAR:
            /* ReqID:TECC4.0 - SIMILAR TO not supported for encrypted columns.
             * Children already recursed by walker. */
            if (expr->name != NIL)
            {
                Node *op_node = (Node *) llast(expr->name);
                char *operator_name = strVal((Value *)op_node);
                if (operator_name != NULL)
                {
                    struct ColumnRef *found_cr;
                    char *col_name;

                    /* Check if left operand references an encrypted column */
                    found_cr = extract_first_column_ref_from_node(expr->lexpr);
                    if (found_cr != NULL && found_cr->fields != NIL)
                    {
                        int nf = list_length(found_cr->fields);
                        Node *f = (Node *) list_nth(found_cr->fields, nf - 1);
                        col_name = strVal((Value *) f);
                        if (col_name != NULL &&
                            where_is_encrypted_column(col_name, statement_data))
                        {
                            tecc_processor_debug(TECC_LOG_WARNING,
                                "SIMILAR TO is not supported for encrypted column \"%s\". "
                                "Use LIKE/ILIKE with SEARCH encryption instead.",
                                col_name);
                            return TECC_WHERE_PROCESSOR_ERROR;
                        }
                    }
                }
            }
            break;

        default:
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Unhandled A_Expr kind: %d", expr->kind);
            break;
    }

    return TECC_WHERE_PROCESSOR_SUCCESS;
}

static TeccWhereClauseProcessorResult
where_process_column_ref(struct ColumnRef *col_ref, TeccStatementData *statement_data)
{
    TeccColumnRefData *col_data = NULL;
    bool expand_result;

    if (col_ref == NULL)
        return TECC_WHERE_PROCESSOR_SUCCESS;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing ColumnRef");

    /* Use expression processor to expand column reference with state context */
    if (statement_data != NULL && statement_data->conn != NULL)
    {
        /* Get TECC state from connection */
        TeccState *state = tecc_get_state(statement_data->conn);
        if (state != NULL)
        {
            expand_result = tecc_expand_column_ref_with_state(col_ref, &col_data, state);
        }
        else
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "TECC state not available for column reference expansion");
            return TECC_WHERE_PROCESSOR_ERROR;
        }
    }
    else
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "No connection available for column reference expansion");
        return TECC_WHERE_PROCESSOR_ERROR;
    }

    if (!expand_result || col_data == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to expand column reference");
        return TECC_WHERE_PROCESSOR_SUCCESS;  /* Non-fatal error */
    }

    /* Check if this is an encrypted column */
    if (col_data->column_name[0] != '\0')
    {
        bool is_encrypted = where_is_encrypted_column(col_data->column_name, statement_data);
        if (is_encrypted)
        {
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Column %s is encrypted", col_data->column_name);
            /* Additional processing for encrypted columns */
        }
    }

    /* Clean up column reference data */
    if (col_data != NULL)
    {
        tecc_column_ref_data_destroy(col_data);
    }

    return TECC_WHERE_PROCESSOR_SUCCESS;
}

/* ReqID:TECC4.0 - Walk subquery tree (SelectStmt or SetOperationStmt) to process
 * WHERE/HAVING clauses. Returns false on error. */
static bool
where_process_subquery_clauses(Node *subquery, TeccStatementData *statement_data)
{
    TeccWhereClauseProcessorResult result;

    if (subquery == NULL)
        return true;

    switch (nodeTag(subquery))
    {
        case FE_T_SelectStmt:
        {
            struct SelectStmt *ss = (struct SelectStmt *)subquery;
            if (ss->whereClause != NULL)
            {
                result = where_process_expr(ss->whereClause, statement_data);
                if (result != TECC_WHERE_PROCESSOR_SUCCESS)
                    return false;
            }
            if (ss->havingClause != NULL)
            {
                result = where_process_expr(ss->havingClause, statement_data);
                if (result != TECC_WHERE_PROCESSOR_SUCCESS)
                    return false;
            }
            return true;
        }
        case FE_T_SetOp:
        {
            struct SetOperationStmt *sop = (struct SetOperationStmt *)subquery;
            if (!where_process_subquery_clauses(sop->larg, statement_data))
                return false;
            if (!where_process_subquery_clauses(sop->rarg, statement_data))
                return false;
            return true;
        }
        default:
            return true;
    }
}

/* ReqID:TECC4.0 - Walk subquery tree to encrypt constants in targetList
 * when testexpr involves an encrypted column. */
static TeccWhereClauseProcessorResult
where_process_subquery_target_consts(Node *subquery, Node *testexpr,
                                     const char *operator_name,
                                     TeccStatementData *statement_data)
{
    TeccWhereClauseProcessorResult result;

    if (subquery == NULL || testexpr == NULL || operator_name == NULL)
        return TECC_WHERE_PROCESSOR_SUCCESS;

    switch (nodeTag(subquery))
    {
        case FE_T_SelectStmt:
        {
            struct SelectStmt *ss = (struct SelectStmt *)subquery;
            if (ss->targetList != NIL)
            {
                ListCell *tlc;
                for (tlc = ss->targetList->head; tlc != NULL; tlc = tlc->next)
                {
                    ResTarget *rt = (ResTarget *)tlc->data.ptr_value;
                    if (rt != NULL && rt->val != NULL &&
                        nodeTag(rt->val) == FE_T_Const)
                    {
                        result = tecc_where_clause_processor_process_comparison(
                            testexpr, rt->val, operator_name, statement_data);
                        if (result != TECC_WHERE_PROCESSOR_SUCCESS)
                            return result;
                    }
                }
            }
            return TECC_WHERE_PROCESSOR_SUCCESS;
        }
        case FE_T_SetOp:
        {
            struct SetOperationStmt *sop = (struct SetOperationStmt *)subquery;
            result = where_process_subquery_target_consts(
                sop->larg, testexpr, operator_name, statement_data);
            if (result != TECC_WHERE_PROCESSOR_SUCCESS)
                return result;
            return where_process_subquery_target_consts(
                sop->rarg, testexpr, operator_name, statement_data);
        }
        default:
            return TECC_WHERE_PROCESSOR_SUCCESS;
    }
}

/*
 * ReqID:TECC4.0 - Extract constant info from a Const node or an
 * A_Expr(UMINUS, Const) combination.  Negative integer constants like
 * -1 are parsed as A_Expr(UMINUS, Const(1)) rather than Const(-1).
 *
 * On success, sets *loc, *dsize, *text (malloc'd), and *ctype.
 * Caller must pg_free(*text).
 */
static bool
where_extract_const_info(Node *node, const char *query,
                         size_t *loc, size_t *dsize,
                         char **text, Oid *ctype)
{
    Const *cnode;

    if (node == NULL || query == NULL || loc == NULL || dsize == NULL ||
        text == NULL || ctype == NULL)
        return false;

    *text = NULL;

    if (nodeTag(node) == FE_T_Const)
    {
        cnode = (Const *) node;
        if (cnode->location < 0 || cnode->data_size <= 0)
            return false;
        *loc = (size_t)cnode->location;
        *dsize = (size_t)cnode->data_size;
        *ctype = cnode->consttype;
        *text = (char *) pg_malloc(cnode->data_size + 1);
        if (*text == NULL) return false;
        memcpy(*text, query + cnode->location, cnode->data_size);
        (*text)[cnode->data_size] = '\0';
        return true;
    }

    /* UMINUS Const case: A_Expr(kind=AEXPR_OP, name="-", rexpr=Const) */
    if (nodeTag(node) == FE_T_A_Expr)
    {
        A_Expr *aexpr = (A_Expr *) node;
        Node *inner;
        if (aexpr->kind != AEXPR_OP)
            return false;
        /* Determine which side holds the operand */
        if (aexpr->lexpr == NULL && aexpr->rexpr != NULL)
            inner = aexpr->rexpr;
        else if (aexpr->rexpr == NULL && aexpr->lexpr != NULL)
            inner = aexpr->lexpr;
        else
            return false; /* not a unary operator */
        if (nodeTag(inner) != FE_T_Const)
            return false;
        cnode = (Const *) inner;
        if (cnode->location < 0 || cnode->data_size <= 0)
            return false;
        if (aexpr->location < 0)
            return false;
        *loc = (size_t)aexpr->location;
        *dsize = (size_t)(cnode->location + cnode->data_size) - (size_t)aexpr->location;
        *ctype = cnode->consttype;
        *text = (char *) pg_malloc(*dsize + 1);
        if (*text == NULL) return false;
        memcpy(*text, query + *loc, *dsize);
        (*text)[*dsize] = '\0';
        return true;
    }

    return false;
}

static bool
where_is_encrypted_column(const char *column_name,
                         TeccStatementData *statement_data)
{
    /* For full table.column checking, use where_is_encrypted_column_full */
    return where_is_encrypted_column_full(NULL, column_name, statement_data);
}

/* ReqID:TECC4.0 - Strict table name matching for encrypted column lookup.
 * When table_name is provided, matches against both table_alias and table_name.
 * When table_name is NULL, only matches if column_name is unique across all
 * encrypted_columns entries. If ambiguous (multiple tables have same column),
 * reports an error to prevent silent data corruption. */
static bool
where_is_encrypted_column_full(const char *table_name,
                              const char *column_name,
                              TeccStatementData *statement_data)
{
    ListCell *lc;
    int match_count;

    if (column_name == NULL)
        return false;

    if (statement_data->encrypted_columns == NIL)
        return false;

    match_count = 0;

    foreach(lc, statement_data->encrypted_columns)
    {
        TeccColumnRefData *encrypted_col = (TeccColumnRefData *) lfirst(lc);
        if (encrypted_col == NULL || encrypted_col->column_name[0] == '\0')
            continue;
        /* Bug 9323 - 子查询/CTE 重命名后的输出列只以别名对外可见，
         * column_name 与 column_alias 任一匹配即算命中。 */
        if (strcmp(encrypted_col->column_name, column_name) != 0 &&
            (encrypted_col->column_alias[0] == '\0' ||
             strcmp(encrypted_col->column_alias, column_name) != 0))
            continue;

        if (table_name != NULL)
        {
            /* ReqID:TECC4.0 - Strict: match table_alias or table_name */
            if (strcmp(encrypted_col->table_alias, table_name) == 0 ||
                strcmp(encrypted_col->table_name, table_name) == 0)
            {
                return true;
            }
        }
        else
        {
            match_count++;
        }
    }

    /* table_name was NULL: only succeed if column name is unique across tables.
     * ReqID:TECC4.0 - In multi-table context (UPDATE FROM, DELETE USING),
     * ambiguous column references are flagged as errors to prevent data
     * corruption. In single-table contexts, column-name-only matching is
     * backward compatible. */
    if (table_name == NULL)
    {
        if (match_count == 1)
        {
            return true;
        }
        if (match_count > 1)
        {
            if (statement_data->multi_table_context)
            {
                tecc_processor_debug(TECC_LOG_ERROR,
                                   "Ambiguous encrypted column \"%s\": found in multiple tables. "
                                   "Use qualified name (table.column or alias.column) to disambiguate.",
                                   column_name);
                return false;
            }
            else
            {
                /* Backward compatibility: fall back to first match */
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Column \"%s\" matched in multiple tables, using first match",
                                   column_name);
                return true;
            }
        }
    }

    return false;
}

static const TeccOperatorSupport *
where_get_operator_support(const char *operator_name)
{
    int i;

    if (operator_name == NULL)
        return NULL;

    for (i = 0; operator_support_table[i].operator_name != NULL; i++)
    {
        if (strcmp(operator_support_table[i].operator_name, operator_name) == 0)
            return &operator_support_table[i];
    }

    return NULL;
}

/*
 * Rewrite WHERE clause in SQL query with encrypted values
 *
 * This function performs the actual rewrite of the WHERE clause parse tree:
 * 1. Extracts the WHERE clause from statement_data->parse_tree
 * 2. Recursively traverses comparison expressions via where_rewrite_expr
 * 3. For each encrypted column comparison:
 *    - Encrypts the constant value using the scheduler
 *    - Replaces the Const node in the parse tree via where_replace_const_value
 *    - Sets consttype to TEXT_ENCOID for backend to dispatch correctly
 *    - Stores encrypted value in raw_values_list for cross-stage use
 * 4. Sets statement_data->needs_rewrite = 1 to signal deparser
 *
 * The modified parse_tree is then deparsed back to SQL text and sent to
 * the server. This is the sole data path for WHERE clause rewrite output.
 */
TeccWhereClauseProcessorResult
tecc_where_clause_processor_rewrite_clause(TeccStatementData *statement_data)
{
    /* Variable declarations at function beginning for ISO C90 compliance */
    Node *where_clause;
    TeccWhereClauseProcessorResult rewrite_result;

    if (statement_data == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid arguments to tecc_where_clause_processor_rewrite_clause");
        return TECC_WHERE_PROCESSOR_ERROR;
    }

    if (!statement_data->where_needs_rewrite)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "WHERE clause does not need rewriting");
        return TECC_WHERE_PROCESSOR_SUCCESS;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Starting WHERE clause rewrite");

    /* Begin - ReqID:TECC4.0 - WHERE子句重写实现 */
    /* WHERE子句重写需要：
     * 1. 识别包含加密列的比较表达式
     * 2. 检查操作符与加密算法的兼容性
     * 3. 重写比较值（类似values_processor）
     * 4. 可能需要重写整个比较表达式
     */

    /* Extract WHERE clause from parse_tree */

    where_clause = where_extract_clause_from_parse_tree(statement_data->parse_tree);
    if (where_clause == NULL)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "No WHERE clause found in parse tree");
        statement_data->where_needs_rewrite = false;
        return TECC_WHERE_PROCESSOR_SUCCESS;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Extracted WHERE clause from parse tree, starting rewrite");

    /* Recursively rewrite WHERE clause expression */
    rewrite_result = where_rewrite_expr(where_clause, statement_data);

    if (rewrite_result == TECC_WHERE_PROCESSOR_SUCCESS)
    {
        
    }
    else
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "WHERE clause rewrite failed with result: %d", rewrite_result);
    }

    /* Reset the flag since we've processed it */
    statement_data->where_needs_rewrite = false;

    return rewrite_result;
}

/*
 * Set operator type for SEARCH algorithm interface extension
 *
 * This function sets the operator type in the crypto context based on the
 * operator name. This is used by the scheduler to select the appropriate
 * SEARCH algorithm interface (basic encryption vs pattern segmentation).
 */
TeccWhereClauseProcessorResult
tecc_where_clause_processor_set_operator_type(
    TeccStatementData *statement_data,
    const char *operator_name)
{
    TeccOperatorType op_type;
    int scenario_type;

    if (statement_data == NULL || operator_name == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid arguments to tecc_where_clause_processor_set_operator_type");
        return TECC_WHERE_PROCESSOR_ERROR;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Setting operator type for operator: %s", operator_name);

    /* Map operator name to TeccOperatorType */
    op_type = TECC_OP_UNKNOWN;
    scenario_type = TECC_SCENARIO_WHERE_CONDITION;

    /* Begin - ReqID:TECC4.0 - 操作符类型映射 */
    if (strcmp(operator_name, "=") == 0)
    {
        op_type = TECC_OP_EQUAL;
        scenario_type = TECC_SCENARIO_WHERE_CONDITION;
    }
    else if (strcmp(operator_name, "<>") == 0 || strcmp(operator_name, "!=") == 0)
    {
        op_type = TECC_OP_NOT_EQUAL;
        scenario_type = TECC_SCENARIO_WHERE_CONDITION;
    }
    else if (strcmp(operator_name, "<") == 0)
    {
        op_type = TECC_OP_LESS_THAN;
        scenario_type = TECC_SCENARIO_WHERE_CONDITION;
    }
    else if (strcmp(operator_name, ">") == 0)
    {
        op_type = TECC_OP_GREATER_THAN;
        scenario_type = TECC_SCENARIO_WHERE_CONDITION;
    }
    else if (strcmp(operator_name, "<=") == 0)
    {
        op_type = TECC_OP_LESS_EQUAL;
        scenario_type = TECC_SCENARIO_WHERE_CONDITION;
    }
    else if (strcmp(operator_name, ">=") == 0)
    {
        op_type = TECC_OP_GREATER_EQUAL;
        scenario_type = TECC_SCENARIO_WHERE_CONDITION;
    }
    else if (strcmp(operator_name, "LIKE") == 0)
    {
        op_type = TECC_OP_LIKE;
        scenario_type = TECC_SCENARIO_LIKE_PATTERN;
    }
    else if (strcmp(operator_name, "NOT LIKE") == 0)
    {
        op_type = TECC_OP_NOT_LIKE;
        scenario_type = TECC_SCENARIO_LIKE_PATTERN;
    }
    else if (strcmp(operator_name, "ILIKE") == 0)
    {
        op_type = TECC_OP_ILIKE;
        scenario_type = TECC_SCENARIO_LIKE_PATTERN;
    }
    else if (strcmp(operator_name, "NOT ILIKE") == 0)
    {
        op_type = TECC_OP_NOT_ILIKE;
        scenario_type = TECC_SCENARIO_LIKE_PATTERN;
    }
    else if (strcmp(operator_name, "IS DISTINCT FROM") == 0)
    {
        op_type = TECC_OP_IS_DISTINCT_FROM;
        scenario_type = TECC_SCENARIO_WHERE_CONDITION;
    }
    else if (strcmp(operator_name, "IS NOT DISTINCT FROM") == 0)
    {
        op_type = TECC_OP_IS_NOT_DISTINCT_FROM;
        scenario_type = TECC_SCENARIO_WHERE_CONDITION;
    }
    /* ReqID:TECC4.0 - HE 标量运算操作符映射（前端 HE 常量加密分支选择） */
    else if (strcmp(operator_name, "+") == 0)
    {
        op_type = TECC_OP_ADD;
        scenario_type = TECC_SCENARIO_WHERE_CONDITION;
    }
    else if (strcmp(operator_name, "-") == 0)
    {
        op_type = TECC_OP_SUB;
        scenario_type = TECC_SCENARIO_WHERE_CONDITION;
    }
    else if (strcmp(operator_name, "*") == 0)
    {
        op_type = TECC_OP_MUL;
        scenario_type = TECC_SCENARIO_WHERE_CONDITION;
    }
    else if (strcmp(operator_name, "^") == 0)
    {
        op_type = TECC_OP_POW;
        scenario_type = TECC_SCENARIO_WHERE_CONDITION;
    }
    else
    {
        /* Unknown operator - use default */
        op_type = TECC_OP_UNKNOWN;
        scenario_type = TECC_SCENARIO_WHERE_CONDITION;
        tecc_processor_debug(TECC_LOG_WARNING,
                           "Unknown operator: %s, using default operator type", operator_name);
    }
    /* End - ReqID:TECC4.0 - 操作符类型映射 */

    /* Set operator type in statement data */
    statement_data->operator_type = op_type;
    statement_data->current_operator_type = op_type;  /* Alias for backward compatibility */
    statement_data->scenario_type = scenario_type;

    

    return TECC_WHERE_PROCESSOR_SUCCESS;
}

/*
 * Extract constant value from A_Const node
 *
 * This function extracts the actual value from an A_Const node and returns
 * it as a string. It handles different value types (integer, string, float, etc.)
 * and performs necessary formatting.
 *
 * Parameters:
 *   const_expr - Pointer to A_Const node
 *   statement_data - Statement data for error reporting
 *
 * Returns:
 *   Dynamically allocated string containing the constant value, or NULL on error
 *   Caller is responsible for freeing the returned string
 */
static char *
where_extract_const_value(Const *const_expr, TeccStatementData *statement_data)
{
    /* Variable declarations at function beginning for ISO C90 compliance */
    char *result = NULL;
    char temp_buffer[256];  /* Temporary buffer for numeric conversions */
    Oid consttype;

    if (const_expr == NULL)
    {
        tecc_processor_debug(TECC_LOG_WARNING,
                           "Const node is NULL in where_extract_const_value");
        return NULL;
    }

    /* Begin - ReqID:TECC4.0 - 常量值提取逻辑 */
    /* Extract value based on constant type */

    /* Check if constant is NULL — no encryption needed for NULL */
    if (const_expr->constisnull)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Skipping NULL constant — no encryption needed");
        return NULL;
    }

    /* Begin - ReqID:TECC4.0 - 增强常量值提取逻辑 */
    /* Extract value based on constant type using consttype field */
    /* We need to handle different PostgreSQL data types */

    /* Check the constant type to determine how to extract the value */
    consttype = const_expr->consttype;

    /* Handle string types (text, varchar, bpchar, etc.) - check consttype first
     * to avoid accessing sval on non-string types (e.g., NUMERIC uses binary data). */
    if (consttype == TEXTOID || consttype == VARCHAROID || consttype == BPCHAROID ||
        consttype == CSTRINGOID || consttype == NAMEOID ||
        consttype == TEXTOID || consttype == 1043 /* varchar */)
    {
        if (const_expr->constvalue.sval != NULL)
        {
            /* String constant */
            char *str_value;
            size_t len;

            str_value = const_expr->constvalue.sval;
            len = strlen(str_value);
            result = (char *)pg_malloc(len + 1);
            if (result != NULL)
            {
                strcpy(result, str_value);
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Extracted string constant (type=%u): %s", consttype, result);
            }
            else
            {
                tecc_processor_set_error(statement_data, TECC_PROCESSOR_MEMORY_ERROR,
                                        "Memory allocation failed for string constant");
            }
        }
        else
        {
            tecc_processor_debug(TECC_LOG_WARNING,
                               "String type with NULL sval (type=%u)", consttype);
        }
    }
    /* Handle integer types (int2, int4, int8) */
    else if (consttype == INT2OID || consttype == INT4OID || consttype == INT8OID ||
             consttype == OIDOID || consttype == REGPROCOID)
    {
        /* Integer constant */
        int64_t int_value = const_expr->constvalue.ival;
        snprintf(temp_buffer, sizeof(temp_buffer), "%ld", (long)int_value);

        result = (char *)pg_malloc(strlen(temp_buffer) + 1);
        if (result != NULL)
        {
            strcpy(result, temp_buffer);
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Extracted integer constant (type=%u): %s", consttype, result);
        }
        else
        {
            tecc_processor_set_error(statement_data, TECC_PROCESSOR_MEMORY_ERROR,
                                    "Memory allocation failed for integer constant");
        }
    }
    /* Handle floating point types (float4, float8, numeric) */
    else if (consttype == FLOAT4OID || consttype == FLOAT8OID || consttype == NUMERICOID)
    {
        /* Float constant */
        double float_value = const_expr->constvalue.dval;

        /* ReqID:TECC4.0 - Use Ryu shortest-decimal to match INSERT path
         * (raw_value.c). float4列用float_to_shortest_decimal_buf对齐float4out,
         * float8/numeric列用double_to_shortest_decimal_buf对齐float8out/
         * numeric_out。后端set_var_from_str会strip尾部零不保留多余dscale,
         * Ryu最短输出恰好与后端一致。 */
        if (consttype == FLOAT4OID)
        {
            float f = (float)float_value;
            float_to_shortest_decimal_buf(f, temp_buffer);
        }
        else
        {
            double_to_shortest_decimal_buf(float_value, temp_buffer);
        }

        result = (char *)pg_malloc(strlen(temp_buffer) + 1);
        if (result != NULL)
        {
            strcpy(result, temp_buffer);
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Extracted float constant (type=%u): %s", consttype, result);
        }
        else
        {
            tecc_processor_set_error(statement_data, TECC_PROCESSOR_MEMORY_ERROR,
                                    "Memory allocation failed for float constant");
        }
    }
    /* Handle boolean type */
    else if (consttype == BOOLOID)
    {
        /* Boolean constant */
        bool bool_value = (const_expr->constvalue.ival != 0);
        const char *bool_str = bool_value ? "true" : "false";

        result = (char *)pg_malloc(strlen(bool_str) + 1);
        if (result != NULL)
        {
            strcpy(result, bool_str);
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Extracted boolean constant (type=%u): %s", consttype, result);
        }
        else
        {
            tecc_processor_set_error(statement_data, TECC_PROCESSOR_MEMORY_ERROR,
                                    "Memory allocation failed for boolean constant");
        }
    }
    /* Handle date/time types - placeholder implementation */
    else if (consttype == DATEOID || consttype == TIMEOID || consttype == TIMESTAMPOID ||
             consttype == TIMESTAMPTZOID || consttype == INTERVALOID)
    {
        /* For date/time types, we need to convert to string representation */
        /* This is a simplified implementation - in production we would need
           to use appropriate conversion functions */
        int64_t time_value = const_expr->constvalue.ival;
        snprintf(temp_buffer, sizeof(temp_buffer), "%ld", (long)time_value);

        result = (char *)pg_malloc(strlen(temp_buffer) + 1);
        if (result != NULL)
        {
            strcpy(result, temp_buffer);
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Extracted date/time constant (type=%u): %s", consttype, result);
        }
        else
        {
            tecc_processor_set_error(statement_data, TECC_PROCESSOR_MEMORY_ERROR,
                                    "Memory allocation failed for date/time constant");
        }
    }
    else
    {
        /* Unknown or unsupported type */
        tecc_processor_debug(TECC_LOG_WARNING,
                           "Unsupported constant type: consttype=%u", consttype);

        /* Try to extract as integer as fallback */
        if (const_expr->constvalue.ival != 0)
        {
            int64_t int_value = const_expr->constvalue.ival;
            snprintf(temp_buffer, sizeof(temp_buffer), "%ld", (long)int_value);

            result = (char *)pg_malloc(strlen(temp_buffer) + 1);
            if (result != NULL)
            {
                strcpy(result, temp_buffer);
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Extracted unknown type as integer fallback: %s", result);
            }
        }
        else
        {
            /* Return empty string as last resort */
            result = (char *)pg_malloc(1);
            if (result != NULL)
            {
                result[0] = '\0';
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Extracted empty constant as fallback");
            }
            else
            {
                tecc_processor_set_error(statement_data, TECC_PROCESSOR_MEMORY_ERROR,
                                        "Memory allocation failed for empty constant");
            }
        }
    }
    /* End - ReqID:TECC4.0 - 增强常量值提取逻辑 */
    /* End - ReqID:TECC4.0 - 常量值提取逻辑 */

    return result;
}

/*
 * WHERE子句重写函数实现
 */

/* Begin - ReqID:TECC4.0 - WHERE子句重写实现 */

/* ReqID:TECC4.0 - Post-visit callback for WHERE clause rewriting */
static bool
where_rewrite_post_visit(Node *node, void *context)
{
    TeccStatementData *statement_data = (TeccStatementData *) context;

    if (nodeTag(node) == FE_T_A_Expr)
    {
        TeccWhereClauseProcessorResult result;
        result = where_rewrite_a_expr((struct A_Expr *) node, statement_data);
        return (result == TECC_WHERE_PROCESSOR_SUCCESS);
    }
    return true;
}

/*
 * Rewrite WHERE clause expression using unified walker.
 * Only A_Expr nodes need post-processing (where_rewrite_comparison).
 * All child recursion is handled by the walker.
 */
static TeccWhereClauseProcessorResult
where_rewrite_expr(Node *expr, TeccStatementData *statement_data)
{
    TeccExprWalkerCallbacks callbacks;

    if (expr == NULL)
        return TECC_WHERE_PROCESSOR_SUCCESS;

    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.post_visit = where_rewrite_post_visit;

    if (!tecc_walk_expression_tree(expr, &callbacks, statement_data))
        return TECC_WHERE_PROCESSOR_ERROR;

    return TECC_WHERE_PROCESSOR_SUCCESS;
}

/*
 * Rewrite A_Expr (operator expression)
 */
static TeccWhereClauseProcessorResult
where_rewrite_a_expr(struct A_Expr *expr, TeccStatementData *statement_data)
{
    TeccWhereClauseProcessorResult result;

    if (expr == NULL)
        return TECC_WHERE_PROCESSOR_SUCCESS;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Rewriting A_Expr with kind: %d", expr->kind);

    /* Process based on expression kind */
    switch (expr->kind)
    {
        case AEXPR_OP:
        case AEXPR_OP_ANY:
        case AEXPR_OP_ALL:
            /* ReqID:TECC4.0 - Unary operators (UMINUS, NOT, etc.)
             * have one NULL side.  Walker visits these as A_Expr children
             * but they don't represent comparisons — nothing to rewrite. */
            if (expr->lexpr == NULL || expr->rexpr == NULL)
                return TECC_WHERE_PROCESSOR_SUCCESS;

            /* Binary operator - check if we need to rewrite comparison */
            if (expr->name != NIL)
            {
                Node *op_node = (Node *) llast(expr->name);
                char *operator_name = strVal((Value *)op_node);
                if (operator_name != NULL)
                {
                    result = where_rewrite_comparison(
                        expr->lexpr, expr->rexpr, operator_name, statement_data);
                    return result;
                }
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Operator name is not a string node");
            }
            break;

        case AEXPR_LIKE:
            /* ReqID:TECC4.0 - LIKE/ILIKE: children already recursed by walker */
            break;

        case AEXPR_SIMILAR:
            /* ReqID:TECC4.0 - SIMILAR TO: children already recursed by walker */
            break;

        default:
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Unhandled A_Expr kind in rewrite: %d", expr->kind);
            break;
    }

    return TECC_WHERE_PROCESSOR_SUCCESS;
}

/*
 * Rewrite comparison expression (column op value)
 */
static TeccWhereClauseProcessorResult
where_rewrite_comparison(Node *lexpr, Node *rexpr,
                        const char *operator_name,
                        TeccStatementData *statement_data)
{
    const char *column_name = NULL;
    const char *table_name = NULL;
    const char *schema_name = NULL;
    bool is_encrypted = false;
    const char *algorithm = NULL;
    const char *key = NULL;
    /* Additional variables for operator compatibility and encryption */
    const TeccOperatorSupport *op_support = NULL;
    bool algorithm_compatible = false;
    TeccUnifiedEncryptResult *encrypt_result_struct = NULL;
    int num_fields;
    const char *kek_name;
    const char *dek_name;
    char *actual_dek_name = NULL;  /* DEK name from tecc_get_column_dek_name */
    int replace_result = 0;

    /* ReqID:TECC4.0 - Unwrap CollateClause from both sides so the
     * underlying ColumnRef can be matched (e.g. name COLLATE "C" = 'val'). */
    while (nodeTag(lexpr) == FE_T_CollateClause)
        lexpr = ((CollateClause *)lexpr)->arg;
    while (nodeTag(rexpr) == FE_T_CollateClause)
        rexpr = ((CollateClause *)rexpr)->arg;

    /* Check if left expression is a column reference */
    if (nodeTag(lexpr) == FE_T_ColumnRef)
    {
        struct ColumnRef *col_ref = (struct ColumnRef *) lexpr;

        /* Extract column information */
        if (col_ref->fields != NIL)
        {
            Node *last_field = (Node *) llast(col_ref->fields);
            column_name = strVal((Value *)last_field);
            if (column_name != NULL)
            {

                /* Try to get table name and schema name if available */
                num_fields = list_length(col_ref->fields);

                if (num_fields >= 2)
                {
                    /* Get the second last element (table name) */
                    int second_last_index = num_fields - 2;
                    Node *second_last = (Node *) list_nth(col_ref->fields, second_last_index);
                    table_name = strVal((Value *)second_last);

                    /* If there are 3 fields, the first is schema name */
                    if (num_fields >= 3)
                    {
                        Node *first_field = (Node *) linitial(col_ref->fields);
                        schema_name = strVal((Value *)first_field);
                    }
                }

                /* ReqID:TECC4.0 - Resolve real table name from encrypted_columns.
                 * ColumnRef may contain alias, not real table name. */
                if (statement_data->encrypted_columns != NIL)
                {
                    ListCell *lc;
                    TeccColumnRefData *found_ed = NULL;
                    int match_cnt = 0;

                    foreach(lc, statement_data->encrypted_columns)
                    {
                        TeccColumnRefData *ed = (TeccColumnRefData *) lfirst(lc);
                        if (ed == NULL || ed->column_name[0] == '\0')
                            continue;
                        /* Bug 9323 - 同上：重命名后的输出列按 column_alias 匹配 */
                        if (strcmp(ed->column_name, column_name) != 0 &&
                            (ed->column_alias[0] == '\0' ||
                             strcmp(ed->column_alias, column_name) != 0))
                            continue;

                        if (table_name != NULL)
                        {
                            if (strcmp(ed->table_alias, table_name) == 0 ||
                                strcmp(ed->table_name, table_name) == 0)
                            {
                                found_ed = ed;
                                match_cnt = 1;
                                break;
                            }
                        }
                        else
                        {
                            found_ed = ed;
                            match_cnt++;
                        }
                    }

                    if (found_ed != NULL && match_cnt == 1)
                    {
                        table_name = found_ed->table_name;
                        schema_name = found_ed->schema_name[0] != '\0' ? found_ed->schema_name : schema_name;
                        /* Bug 9323 - 命中可能来自 column_alias，取算法/密钥必须按
                         * 底层列名查缓存 */
                        column_name = found_ed->column_name;
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                           "Resolved table name from statement encrypted_columns: %s.%s",
                                           schema_name ? schema_name : "public", table_name);
                    }
                    else if (match_cnt > 1)
                    {
                        if (statement_data->multi_table_context)
                        {
                            tecc_processor_debug(TECC_LOG_ERROR,
                                               "Ambiguous encrypted column \"%s\" - use qualified name",
                                               column_name);
                        }
                        else if (found_ed != NULL)
                        {
                            /* Backward compatible: use first match */
                            table_name = found_ed->table_name;
                            schema_name = found_ed->schema_name[0] != '\0' ? found_ed->schema_name : schema_name;
                            /* Bug 9323 - 同上：回填底层列名再查缓存 */
                            column_name = found_ed->column_name;
                        }
                    }
                }

                /* Begin - ReqID:TECC4.0 - 操作符兼容性检查 */
                /* First, check operator compatibility */
                op_support = where_get_operator_support(operator_name);
                if (op_support == NULL)
                {
                    tecc_processor_debug(TECC_LOG_WARNING,
                                       "Operator not supported for encrypted columns: %s",
                                       operator_name);
                    /* Continue processing but note that operator may not be compatible */
                }
                else
                {
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                       "Operator %s supports: DET=%d, OPE=%d, SEARCH=%d, required_algo=%s",
                                       operator_name,
                                       op_support->supports_deterministic,
                                       op_support->supports_order_preserving,
                                       op_support->supports_searchable,
                                       op_support->required_algorithm ? op_support->required_algorithm : "none");
                }
                /* End - ReqID:TECC4.0 - 操作符兼容性检查 */

                /* Check if column is encrypted */
                is_encrypted = where_is_encrypted_column_full(table_name, column_name, statement_data);

                if (is_encrypted)
                {
                    /* ReqID:TECC4.0 - Reset operator_type for each comparison.
                     * Without this, rewrite phase reuses stale operator_type from
                     * processing phase's last A_Expr, causing wrong algorithm
                     * selection for columns with mixed DET+OPE support. */
                    if (operator_name != NULL)
                        tecc_where_clause_processor_set_operator_type(statement_data, operator_name);

                    /* Get encryption algorithm and key for this column */
                    algorithm = tecc_values_processor_get_column_algorithm_with_state(
                        statement_data, schema_name, table_name, column_name, operator_name);
                    key = tecc_values_processor_get_column_key_with_state(
                        statement_data, schema_name, table_name, column_name);

                    if (algorithm != NULL && key != NULL)
                    {
                        /* Begin - ReqID:TECC4.0 - 算法与操作符兼容性验证 */
                        /* Check if algorithm is compatible with operator */
                        algorithm_compatible = false;

                        if (op_support != NULL)
                        {
                            if (strcmp(algorithm, "DET") == 0 && op_support->supports_deterministic)
                            {
                                algorithm_compatible = true;
                            }
                            else if (strcmp(algorithm, "OPE") == 0 && op_support->supports_order_preserving)
                            {
                                algorithm_compatible = true;
                            }
                            else if (strcmp(algorithm, "SEARCH") == 0 && op_support->supports_searchable)
                            {
                                algorithm_compatible = true;
                            }
                            else if (op_support->required_algorithm != NULL &&
                                     strcmp(algorithm, op_support->required_algorithm) == 0)
                            {
                                algorithm_compatible = true;
                            }

                            if (!algorithm_compatible)
                            {
                                tecc_processor_debug(TECC_LOG_WARNING,
                                                   "Algorithm %s is not compatible with operator %s for column %s.%s",
                                                   algorithm, operator_name,
                                                   table_name ? table_name : "(unknown)",
                                                   column_name);
                                /* We'll still try to encrypt, but warn the user */
                            }
                            else
                            {
                                tecc_processor_debug(TECC_LOG_DEBUG1,
                                                   "Algorithm %s is compatible with operator %s",
                                                   algorithm, operator_name);
                            }
                        }
                        else
                        {
                            /* No operator support info available, assume compatible */
                            algorithm_compatible = true;
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                               "No operator support info, assuming algorithm %s is compatible",
                                               algorithm);
                        }
                        /* End - ReqID:TECC4.0 - 算法与操作符兼容性验证 */

                        /* Check if right expression is a constant that needs encryption */
                        if (nodeTag(rexpr) == FE_T_Const)
                        {
                            Const *const_expr = (Const *) rexpr;
                            char *const_value;

                            /* ReqID:TECC4.0 - Skip if already encrypted.
                             * BETWEEN SYMMETRIC shares Const nodes across the
                             * decomposed OR/AND tree, causing the same node to be
                             * visited twice.  After the first visit, consttype is
                             * set to an encrypted OID (50000-50017). */
                            if (const_expr->consttype >= 50000 &&
                                const_expr->consttype <= 50017)
                            {
                                tecc_processor_debug(TECC_LOG_DEBUG1,
                                                   "Const already encrypted (type=%u) — skipping shared node",
                                                   const_expr->consttype);
                                return TECC_WHERE_PROCESSOR_SUCCESS;
                            }

                            const_value = where_extract_const_value(const_expr, statement_data);

                            if (const_value != NULL)
                            {
                                /* Create TeccRawValue for encryption */
                                TeccRawValue *raw_value = tecc_raw_value_create_from_string(const_value, strlen(const_value));

                                if (raw_value != NULL)
                                {
                                    /* Set location/data_size from Const node for SQL text replacement.
                                     * Critical for values_processor to find and replace the correct
                                     * plaintext in the original SQL. ReqID:TECC4.0 */
                                    if (const_expr != NULL && const_expr->location >= 0 && const_expr->data_size > 0)
                                    {
                                        raw_value->location = (size_t)const_expr->location;
                                        raw_value->data_size = (size_t)const_expr->data_size;
                                    }

                                    /* ReqID:TECC4.0 - Set column_type for ::type_name cast.
                                     * Look up column's actual encryption type from cache first,
                                     * so varchar_enc/bpchar_enc get correct type rather than
                                     * the generic text_enc derived from the string constant. */
                                    {
                                        const char *enc_type_name = NULL;

                                        if (statement_data->conn != NULL
                                            && table_name != NULL
                                            && column_name != NULL)
                                        {
                                            TeccState *s = tecc_get_state(statement_data->conn);
                                            const char *db = PQdb(statement_data->conn);
                                            if (s != NULL && db != NULL)
                                            {
                                                const char *schema = schema_name;
                                                if (schema == NULL || schema[0] == '\0')
                                                    schema = tecc_cache_get_guc_param(
                                                        s, "current_schema");
                                                if (schema != NULL)
                                                {
                                                    TeccColumnCacheItem *col_cache =
                                                        tecc_cache_get_encrypted_column_by_name(
                                                            s, db, schema, table_name, column_name);
                                                    if (col_cache != NULL)
                                                        enc_type_name = get_enc_type_name_from_base_oid(
                                                            col_cache->original_type_oid);
                                                }
                                            }
                                        }
                                        /* Cache miss: leave enc_type_name=NULL.
                                         * Do NOT fall back to constant's type. */
                                        tecc_raw_value_set_column_info(raw_value, column_name,
                                                                       enc_type_name, InvalidOid);
                                    }
                                    if (table_name != NULL)
                                    {
                                        tecc_raw_value_set_table_info(raw_value, schema_name, table_name);
                                    }

                                    /* Begin - ReqID:TECC4.0 - 使用调度器模块加密并修改解析树 */
                                    /* Use crypto scheduler to encrypt the value */
                                    encrypt_result_struct = NULL;

                                    /* Get KEK and DEK names for the column */
                                    kek_name = where_get_column_kek_name(statement_data, schema_name, table_name, column_name);
                                    if (kek_name == NULL)
                                    {
                                        tecc_processor_debug(TECC_LOG_ERROR,
                                                           "Cannot resolve KEK name for column %s.%s, aborting rewrite",
                                                           table_name ? table_name : "?", column_name ? column_name : "?");
                                        return TECC_WHERE_PROCESSOR_ERROR;
                                    }
                                    /* Use tecc_get_column_dek_name to get actual DEK name from cache */
                                    actual_dek_name = tecc_get_column_dek_name(
                                        statement_data, schema_name, table_name, column_name);
                                    if (actual_dek_name != NULL) {
                                        dek_name = actual_dek_name;
                                    } else if (key != NULL) {
                                        dek_name = key;
                                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                                           "Using fallback key for column %s.%s",
                                                           table_name, column_name);
                                    } else {
                                        tecc_processor_debug(TECC_LOG_ERROR,
                                                           "Cannot resolve DEK name for column %s.%s, aborting rewrite",
                                                           table_name ? table_name : "?", column_name ? column_name : "?");
                                        return TECC_WHERE_PROCESSOR_ERROR;
                                    }

                                    /* ReqID:TECC4.0 - HE 标量运算常量加密
                                     * （改写阶段与 process 阶段同构）：
                                     * + / - → EC-ElGamal（EG_ADD 段），
                                     * * → FF ElGamal（EG_MUL 段）。 */
                                    if (statement_data->operator_type == TECC_OP_ADD ||
                                        statement_data->operator_type == TECC_OP_SUB)
                                    {
                                        encrypt_result_struct = tecc_unified_encrypt_he_constant(
                                            statement_data->conn,
                                            const_value,
                                            table_name,
                                            column_name,
                                            schema_name,
                                            true    /* use_he_add: EG_ADD 段 */
                                        );
                                    }
                                    else if (statement_data->operator_type == TECC_OP_MUL)
                                    {
                                        encrypt_result_struct = tecc_unified_encrypt_he_constant(
                                            statement_data->conn,
                                            const_value,
                                            table_name,
                                            column_name,
                                            schema_name,
                                            false   /* FF ElGamal: EG_MUL 段 */
                                        );
                                    }
                                    /* Determine which encryption function to call based on operator type */
                                    else if (statement_data->operator_type == TECC_OP_LIKE ||
                                        statement_data->operator_type == TECC_OP_NOT_LIKE ||
                                        statement_data->operator_type == TECC_OP_ILIKE ||
                                        statement_data->operator_type == TECC_OP_NOT_ILIKE)
                                    {
                                        /* ReqID:TECC4.0 - Use operator-specific encryption for WHERE */
                                        

                                        encrypt_result_struct = tecc_unified_encrypt_for_where(
                                            statement_data->conn,
                                            const_value,
                                            table_name,
                                            column_name,
                                            schema_name,
                                            kek_name,
                                            dek_name,
                                            statement_data->operator_type,
                                            statement_data->like_escape_char,
                                            NULL
                                        );
                                    }
                                    else
                                    {
                                        /* ReqID:TECC4.0 - Use operator-specific encryption for WHERE */


                                        encrypt_result_struct = tecc_unified_encrypt_for_where(
                                            statement_data->conn,
                                            const_value,
                                            table_name,
                                            column_name,
                                            schema_name,
                                            kek_name,
                                            dek_name,
                                            statement_data->operator_type,
                                            0,
                                            NULL
                                        );
                                    }

                                    if (encrypt_result_struct != NULL &&
                                        encrypt_result_struct->error_code == TECC_CRYPTO_SUCCESS &&
                                        encrypt_result_struct->hex_string != NULL)
                                    {
                                        

                                        /* Store encrypted value in raw_value for rewrite phase */
                                        if (raw_value->processed.encrypted_value != NULL)
                                        {
                                            pg_free(raw_value->processed.encrypted_value);
                                        }
                                        raw_value->processed.encrypted_value = tecc_processor_strdup(encrypt_result_struct->hex_string);
                                        raw_value->state = TECC_RAW_VALUE_STATE_PROCESSED;

                                        /* Add to raw_values_list for rewrite phase.
                                         * AEXPR_OP: process phase already added it (dup check below).
                                         * AEXPR_LIKE: process phase did NOT add it — must add here.
                                         * ReqID:TECC4.0 */
                                        if (statement_data->raw_values_list == NULL)
                                            statement_data->raw_values_list = tecc_raw_values_list_create(16);
                                        if (statement_data->raw_values_list != NULL)
                                        {
                                            bool dup = false;
                                            size_t vi;
                                            for (vi = 0; vi < statement_data->raw_values_list->num_values; vi++)
                                            {
                                                TeccRawValue *e = statement_data->raw_values_list->values[vi];
                                                if (e && e->location == raw_value->location && e->data_size == raw_value->data_size)
                                                { dup = true; break; }
                                            }
                                            if (!dup)
                                            {
                                                if (tecc_raw_values_list_add(statement_data->raw_values_list, raw_value) < 0)
                                                    tecc_raw_value_destroy(raw_value);
                                            }
                                            else
                                            {
                                                tecc_raw_value_destroy(raw_value);
                                            }
                                        }
                                        else
                                        {
                                            tecc_raw_value_destroy(raw_value);
                                        }
                                        raw_value = NULL;

                                        /* Replace the constant value in the parse tree with encrypted value */
                                        /* ReqID:TECC4.0 - Map original base type OID to correct encrypted type OID */
                                        {
                                            Oid enc_oid = where_get_encrypted_type_oid(const_expr->consttype);
                                            replace_result = where_replace_const_value(
                                                &rexpr,  /* Pointer to the right expression node */
                                                encrypt_result_struct->hex_string,  /* Encrypted hex string */
                                                statement_data,
                                                enc_oid  /* encrypted_type_oid */
                                            );
                                        }

                                        if (replace_result == 0)
                                        {
                                            
                                            statement_data->needs_rewrite = 1;
                                        }
                                        else
                                        {
                                            tecc_processor_debug(TECC_LOG_ERROR,
                                                               "Failed to replace constant value in parse tree");
                                        }

                                        /* Free encryption result */
                                        tecc_unified_free_encrypt_result(encrypt_result_struct);
                                    }
                                    else
                                    {
                                        tecc_processor_debug(TECC_LOG_ERROR,
                                                           "Failed to encrypt WHERE clause value for column: %s",
                                                           column_name);

                                        if (encrypt_result_struct != NULL)
                                        {
                                            tecc_processor_debug(TECC_LOG_ERROR,
                                                               "Encryption error: %s", encrypt_result_struct->error_msg);
                                            tecc_unified_free_encrypt_result(encrypt_result_struct);
                                        }
                                    }

                                    /* Clean up raw value if not added to list */
                                    if (raw_value != NULL)
                                    {
                                        tecc_raw_value_destroy(raw_value);
                                    }
                                    /* End - ReqID:TECC4.0 - 使用调度器模块加密并修改解析树 */
                                }
                                else
                                {
                                    tecc_processor_debug(TECC_LOG_ERROR,
                                                       "Failed to create raw value for encryption");
                                }

                                /* Free the extracted constant value */
                                pg_free(const_value);
                            }
                        }
                        else if (nodeTag(rexpr) == FE_T_Param)
                        {
                            /* Parameter reference - will be handled by parameter processing */
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                               "Parameter reference in WHERE clause - will be handled by parameter processor");
                        }
                        else
                        {
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                               "Right expression is not a constant or parameter: type=%d",
                                               nodeTag(rexpr));
                        }
                    }
                    else
                    {
                        tecc_processor_debug(TECC_LOG_WARNING,
                                           "Could not get algorithm or key for encrypted column: %s.%s",
                                           table_name ? table_name : "(unknown)",
                                           column_name);
                    }
                }
            }
        }
    }
    else
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Left expression is not a column reference: type=%d",
                           nodeTag(lexpr));
    }

    /* Free allocated DEK name memory */
    if (actual_dek_name != NULL) {
        pg_free(actual_dek_name);
        actual_dek_name = NULL;
    }

    return TECC_WHERE_PROCESSOR_SUCCESS;
}

/* End - ReqID:TECC4.0 - WHERE子句重写实现 */

/*
 * Extract WHERE clause from parse tree
 *
 * This function extracts the WHERE clause from the parse tree based on
 * the statement type. It handles SELECT, UPDATE, and DELETE statements.
 * If there are multiple statements in the parse tree, it processes the
 * first statement that has a WHERE clause.
 */
static Node *
where_extract_clause_from_parse_tree(List *parse_tree)
{
    ListCell *lc;

    if (parse_tree == NIL)
        return NULL;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Extracting WHERE clause from parse tree with %d statements",
                       list_length(parse_tree));

    /* Process all statements in the parse tree */
    foreach(lc, parse_tree)
    {
        Node *stmt_node = (Node *) lfirst(lc);
        Node *where_clause = NULL;

        if (stmt_node == NULL)
            continue;

        /* Extract WHERE clause based on statement type */
        switch (nodeTag(stmt_node))
        {
            case FE_T_SelectStmt:
                {
                    struct SelectStmt *select_stmt = (struct SelectStmt *) stmt_node;
                    where_clause = select_stmt->whereClause;
                    break;
                }

            case FE_T_UpdateStmt:
                {
                    struct UpdateStmt *update_stmt = (struct UpdateStmt *) stmt_node;
                    where_clause = update_stmt->whereClause;
                    break;
                }

            case FE_T_DeleteStmt:
                {
                    struct DeleteStmt *delete_stmt = (struct DeleteStmt *) stmt_node;
                    where_clause = delete_stmt->whereClause;
                    break;
                }

            default:
                /* Other statement types don't have WHERE clauses */
                where_clause = NULL;
                break;
        }

        /* If we found a WHERE clause, return it */
        if (where_clause != NULL)
        {
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Found WHERE clause in statement type: %d",
                               nodeTag(stmt_node));
            return where_clause;
        }
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "No WHERE clause found in any statement in parse tree");
    return NULL;
}

/*
 * where_get_column_kek_name - Get KEK name for a column from cache
 *
 * Parameters:
 *   statement_data - Statement data containing connection context
 *   schema_name - Schema name (can be NULL)
 *   table_name - Table name
 *   column_name - Column name
 *
 * Returns:
 *   KEK name string (static buffer, do not free) or NULL on failure
 */
static const char *
where_get_column_kek_name(TeccStatementData *statement_data,
                         const char *schema_name,
                         const char *table_name,
                         const char *column_name)
{
    /* Variable declarations at function beginning for ISO C90 compliance */
    TeccState *state = NULL;
    const char *database_name = NULL;
    const char *actual_schema_name = schema_name;
    TeccColumnCacheItem *column_cache = NULL;
    TeccDekCacheItem *dek_cache = NULL;
    TeccKekCacheItem *kek_cache = NULL;
    static char kek_name_buffer[NAMEDATALEN];

    /* Check parameters */
    if (statement_data == NULL || table_name == NULL || column_name == NULL)
    {
        tecc_processor_debug(TECC_LOG_WARNING,
                           "where_get_column_kek_name: Invalid parameters");
        return NULL;
    }

    /* Get state and database name from connection */
    if (statement_data->conn != NULL)
    {
        state = tecc_get_state(statement_data->conn);
        database_name = PQdb(statement_data->conn);
    }

    if (state == NULL || database_name == NULL)
    {
        tecc_processor_debug(TECC_LOG_WARNING,
                           "where_get_column_kek_name: Cannot get state or database name");
        return NULL;
    }

    /* If schema_name is NULL, get current schema from cache */
    if (actual_schema_name == NULL)
    {
        actual_schema_name = tecc_cache_get_guc_param(state, "current_schema");
        if (actual_schema_name == NULL || actual_schema_name[0] == '\0')
        {
            tecc_processor_debug(TECC_LOG_WARNING,
                               "where_get_column_kek_name: Cannot get current schema");
            return NULL;
        }
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "where_get_column_kek_name: Looking up column %s.%s.%s",
                       database_name, actual_schema_name, column_name);

    /* Get column cache item */
    column_cache = tecc_cache_get_encrypted_column_by_name(
        state, database_name, actual_schema_name, table_name, column_name);

    if (column_cache == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "where_get_column_kek_name: Column %s.%s not found in cache (db=%s, schema=%s)",
                           table_name, column_name,
                           database_name ? database_name : "NULL",
                           actual_schema_name ? actual_schema_name : "NULL");
        return NULL;
    }

    /* Check if DEK name is available.
     * ReqID:TECC4.0 - 纯 HE 列（无对称 DEK，dek_name 为空）回退到
     * he_add_dek_name / he_mul_dek_name（与 tecc_get_column_dek_name 一致），
     * 保证 HE 标量运算常量加密路径能解析出 KEK。用局部变量，不改动缓存项。 */
    {
        const char *effective_dek_name = column_cache->dek_name;

        if ((effective_dek_name == NULL || effective_dek_name[0] == '\0') &&
            column_cache->he_add_dek_name != NULL && column_cache->he_add_dek_name[0] != '\0')
            effective_dek_name = column_cache->he_add_dek_name;
        else if ((effective_dek_name == NULL || effective_dek_name[0] == '\0') &&
                 column_cache->he_mul_dek_name != NULL && column_cache->he_mul_dek_name[0] != '\0')
            effective_dek_name = column_cache->he_mul_dek_name;

        if (effective_dek_name == NULL || effective_dek_name[0] == '\0')
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "where_get_column_kek_name: DEK name is empty for column %s.%s (cache: table_oid=%u, enc_type=%d)",
                               table_name, column_name,
                               column_cache->table_oid, column_cache->encryption_type);
            return NULL;
        }

        /* Get DEK cache item */
        dek_cache = tecc_cache_get_dek_by_name_search_path(state, effective_dek_name);
        if (dek_cache == NULL)
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "where_get_column_kek_name: DEK '%s' not found in cache (dek_name from column cache: %s)",
                               effective_dek_name, column_cache->dek_name ? column_cache->dek_name : effective_dek_name);
            return NULL;
        }
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "where_get_column_kek_name: DEK '%s' found, encrypted_key=%p, key_length=%zu, kek_name=%s, kek_oid=%u, status=%d",
                       column_cache->dek_name,
                       (void*)dek_cache->encrypted_key, dek_cache->key_length,
                       dek_cache->kek_name ? dek_cache->kek_name : "NULL",
                       dek_cache->kek_oid, dek_cache->status);

    /* Get KEK cache item by OID, fallback to name */
    kek_cache = tecc_cache_get_kek_by_oid(state, dek_cache->kek_oid);
    if (kek_cache == NULL && dek_cache->kek_name != NULL)
        kek_cache = tecc_cache_get_kek_by_name_search_path(state, dek_cache->kek_name);
    if (kek_cache == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "where_get_column_kek_name: KEK OID %u (name=%s) not found in cache",
                           dek_cache->kek_oid,
                           dek_cache->kek_name ? dek_cache->kek_name : "NULL");
        return NULL;
    }

    /* Check if KEK name is available */
    if (kek_cache->key_name == NULL || kek_cache->key_name[0] == '\0')
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "where_get_column_kek_name: KEK name is empty (kek_oid=%u, kek_name_field=%s)",
                           dek_cache->kek_oid,
                           kek_cache->key_name ? kek_cache->key_name : "NULL");
        return NULL;
    }

    /* Copy KEK name to static buffer */
    strncpy(kek_name_buffer, kek_cache->key_name, NAMEDATALEN - 1);
    kek_name_buffer[NAMEDATALEN - 1] = '\0';

    

    return kek_name_buffer;
}

/* ReqID:TECC4.0 - Map base type OID to encrypted type OID */
static Oid
where_get_encrypted_type_oid(Oid base_type_oid)
{
    switch (base_type_oid)
    {
        case INT2OID:    return INT2_ENCOID;
        case INT4OID:    return INT4_ENCOID;
        case INT8OID:    return INT8_ENCOID;
        case FLOAT4OID:  return FLOAT4_ENCOID;
        case FLOAT8OID:  return FLOAT8_ENCOID;
        case NUMERICOID: return NUMERIC_ENCOID;
        case VARCHAROID: return VARCHAR_ENCOID;
        case BPCHAROID:  return BPCHAR_ENCOID;
        case TEXTOID:    return TEXT_ENCOID;
        default:         return TEXT_ENCOID;  /* fallback */
    }
}

/*
 * Replace constant value in parse tree with encrypted value
 *
 * This function replaces the value in a Const node with an encrypted value.
 * It creates a new Const node with the encrypted string value.
 *
 * Parameters:
 *   expr_ptr - Pointer to the expression node pointer (allows modification)
 *   encrypted_value - The encrypted value to replace with
 *   statement_data - Statement data for error reporting
 *
 * Returns:
 *   0 on success, -1 on error
 */
static int
where_replace_const_value(Node **expr_ptr, const char *encrypted_value,
                         TeccStatementData *statement_data,
                         Oid encrypted_type_oid)
{
    /* Variable declarations at function beginning for ISO C90 compliance */
    Node *expr = NULL;
    Const *const_expr = NULL;
    char *new_str_value = NULL;

    if (expr_ptr == NULL || *expr_ptr == NULL || encrypted_value == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid arguments to where_replace_const_value");
        return -1;
    }

    /* Begin - ReqID:TECC4.0 - 常量值替换实现 */
    expr = *expr_ptr;

    /* Check if this is a Const node */
    if (nodeTag(expr) != FE_T_Const)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Expression is not a Const node, cannot replace value: type=%d",
                           nodeTag(expr));
        return -1;
    }

    const_expr = (Const *)expr;
    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Replacing constant value in Const node (type=%u)",
                       const_expr->consttype);

    /* Create a new string value for the encrypted data */
    /* Note: We need to allocate new memory for the string value */
    new_str_value = (char *)pg_malloc(strlen(encrypted_value) + 1);
    if (new_str_value == NULL)
    {
        tecc_processor_set_error(statement_data, TECC_PROCESSOR_MEMORY_ERROR,
                                "Memory allocation failed for encrypted value");
        return -1;
    }

    strcpy(new_str_value, encrypted_value);

    /* Free the old string value if it exists - only for real string types */
    /* (NUMERIC等类型的constvalue.sval不是malloc字符串) */
    if (const_expr->consttype == TEXTOID || const_expr->consttype == VARCHAROID ||
        const_expr->consttype == BPCHAROID || const_expr->consttype == CSTRINGOID)
    {
        if (const_expr->constvalue.sval != NULL)
        {
            pg_free(const_expr->constvalue.sval);
        }
    }

    /* Update the Const node with the new encrypted value */
    const_expr->constvalue.sval = new_str_value;

    /* Update the constant type to the encrypted type OID,
     * dynamically determined from the column's encryption type */
    if (encrypted_type_oid != InvalidOid)
        const_expr->consttype = encrypted_type_oid;
    else
        const_expr->consttype = TEXT_ENCOID;

    /* Mark that the constant is not NULL */
    const_expr->constisnull = false;

    /* Update the location if needed (optional) */
    /* const_expr->location = ... */

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Set constant type to TEXT_ENCOID (%u) for encrypted value", TEXT_ENCOID);

    

    return 0;
    /* End - ReqID:TECC4.0 - 常量值替换实现 */
}

/*
 * where_check_algorithm_for_operator - Check if the encrypted column's algorithm
 * supports the given comparison operator.
 *
 * Returns TECC_WHERE_PROCESSOR_SUCCESS if compatible, error code otherwise.
 */
static TeccWhereClauseProcessorResult
where_check_algorithm_for_operator(
    const char *operator_name,
    TeccColumnCacheItem *column_item,
    TeccStatementData *statement_data)
{
    const TeccOperatorSupport *op_support = NULL;

    if (operator_name == NULL || column_item == NULL || statement_data == NULL)
    {
        return TECC_WHERE_PROCESSOR_ERROR;
    }

    /* Check if operator is in support table */
    op_support = where_get_operator_support(operator_name);
    if (op_support == NULL)
    {
        return TECC_WHERE_PROCESSOR_SUCCESS;
    }

    /* Check if column has required algorithms using encryption_type bitmask.
     * Bitmask values (from tecc_column_cache.c):
     *   0x01 = HG_ENCRYPTION_TYPE_DET, 0x02 = HG_ENCRYPTION_TYPE_OPE,
     *   0x04 = HG_ENCRYPTION_TYPE_SEARCH */

    /* For equality operators (=, <>), either DET or OPE can satisfy the requirement.
     * For ordering operators (<, >, <=, >=), only OPE can satisfy.
     * For pattern matching, only SEARCH can satisfy.
     * encryption_type==0 means RND only — no DET/OPE/SEARCH available. ReqID:TECC4.0 */
    if (op_support->supports_deterministic && op_support->supports_order_preserving)
    {
        /* This operator (typically =, <>) can be satisfied by EITHER DET or OPE.
         * Only fail if NEITHER algorithm is available.
         * encryption_type==0 means RND only — no DET/OPE available. */
        if (!(column_item->encryption_type & 0x01) && !(column_item->encryption_type & 0x02))
        {
            tecc_processor_debug(TECC_LOG_WARNING,
                               "Column does not support DET or OPE algorithms required by operator %s",
                               operator_name);
            return TECC_WHERE_PROCESSOR_MISSING_DET_OPE;
        }
    }
    else
    {
        /* Operator requires specific algorithm(s). Check each separately.
         * encryption_type==0 means RND only — no DET/OPE available. */
        if (op_support->supports_deterministic &&
            !(column_item->encryption_type & 0x01))
        {
            tecc_processor_debug(TECC_LOG_WARNING,
                               "Column does not support DET algorithm required by operator %s",
                               operator_name);
            return TECC_WHERE_PROCESSOR_MISSING_DET;
        }

        if (op_support->supports_order_preserving && !(column_item->encryption_type & 0x02))
        {
            tecc_processor_debug(TECC_LOG_WARNING,
                               "Column does not support OPE algorithm required by operator %s",
                               operator_name);
            return TECC_WHERE_PROCESSOR_MISSING_OPE;
        }
    }

    if (op_support->supports_searchable && !(column_item->encryption_type & 0x04))
    {
        tecc_processor_debug(TECC_LOG_WARNING,
                           "Column does not support SEARCH algorithm required by operator %s",
                           operator_name);
        return TECC_WHERE_PROCESSOR_MISSING_SEARCH;
    }

    return TECC_WHERE_PROCESSOR_SUCCESS;
}