/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_grammar.y
 * Abstract:
 *   Enhanced grammar for TECC frontend parser with full SQL support.
 *   Uses tecc_nodes.h data structures for compatibility with processor module.
 *
 * Authored by <wangxp@highgo.com>, 20260306.
 * Updated for TECC4.0 compatibility, 20260408.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/frontend_parser/tecc_grammar.y
 */

%union {
    long long ival;
    char *str;
    double dval;
    int bval;  /* Use int for bool, will be converted in code */
    void *node;
    void *list;
    void *range;
}

%define api.prefix {tecc_}
%define api.token.prefix {TECC_TOK_}
%define parse.error verbose

/* Destructors for semantic values - disabled to avoid conflicts */
/* %destructor { if ($$) freeNode((Node *)$$); } <node> */
/* %destructor { if ($$) free_node_list((List *)$$); } <list> */
/* %destructor { if ($$) tecc_pfree($$); } <str> */

%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

/* Include TECC node definitions */
#include "tecc_nodes.h"
/* Include list functions (must be before tecc_parser.h which uses List type) */
#include "../tecc_common/tecc_frontend_list.h"
#include "tecc_parser.h"
/* Include logging */
#include "../tecc_common/tecc_logging.h"

/* Bison error reporting function - defined by Bison in tecc_grammar.c */
void tecc_yyerror(const char *msg);
void tecc_error(const char *msg);

/* Use our memory allocation functions for Bison */
#define YYMALLOC tecc_palloc
#define YYFREE tecc_pfree

/* Define TECC_LTYPE before including headers */
#if ! defined TECC_LTYPE && ! defined TECC_LTYPE_IS_DECLARED
typedef struct TECC_LTYPE
{
    int first_line;
    int first_column;
    int last_line;
    int last_column;
} TECC_LTYPE;
#define TECC_LTYPE_IS_DECLARED 1
#endif

/* Use extern "C" for C++ compatibility */
#ifdef __cplusplus
extern "C" {
#endif

/* Global variable for parse tree */
List *tecc_parsetree = NIL;

/* Declare parser functions */
int tecc_lex(void);
void tecc_yyerror(const char *msg);

/* Helper functions for node creation */
static Node *make_column_ref_from_list(List *fields, int location);
static Node *make_func_call_from_list(List *funcname, List *args, int location);
static Node *make_a_expr(int kind, List *name, Node *lexpr, Node *rexpr, int location);
static Node *make_a_expr_escape(int kind, List *name, Node *lexpr, Node *rexpr, Node *escape, int location);
static Node *make_bool_expr(int boolop, List *args, int location);
/* ReqID:TECC4.0 - Expand `expr op ANY/SOME/ALL(ARRAY[...])` into OR/AND of
 * element comparisons so each constant goes through the normal encryption path. */
static Node *make_any_all_array_expr(void *op_name, void *lhs, List *elements, int use_or, int location);
static Node *make_null_test(Node *arg, int nulltesttype, int location);
static List *convert_string_list_to_restargets(List *string_list, int location);

/* Node copy function */
extern Node *tecc_copy_node(const Node *node);

/* Simple helper to create a string value node */
static Value *makeString(const char *str) {
    Value *v = (Value *)tecc_palloc(sizeof(Value));
    v->type = FE_T_String;
    v->val.str = tecc_pstrdup(str);
    v->location = 0;
    return v;
}


/* ReqID:TECC4.0 - lcons: prepend element to front of list */
static List *lcons(void *datum, List *list) {
    List *new_list = list_make1(datum);
    if (list != NIL && list != NULL) {
        new_list->head->next = list->head;
        new_list->tail = list->tail;
        new_list->length += list->length;
        new_list->next = list->next;
    }
    return new_list;
}
/* Helper to free a list containing nodes */
static pg_attribute_unused() void free_node_list(List *list) {
    if (!list) return;
    {
        ListCell *cell = list->head;
        while (cell) {
            ListCell *next = cell->next;
            if (cell->data.ptr_value) {
                freeNode((Node *)cell->data.ptr_value);
            }
            tecc_pfree(cell);
            cell = next;
        }

        tecc_pfree(list);
    }
}

/* Helper to create ColumnDef for ALTER COLUMN TYPE with collation */
static Node *makeColumnDefForAlter(const char *colname, TypeName *typeName, Node *opt_collate) {
    ColumnDef *coldef = makeColumnDef(0, 0); /* location will be set by caller */
    coldef->colname = tecc_pstrdup(colname);
    coldef->typeName = typeName;
    coldef->constraints = NIL;
    coldef->collClause = opt_collate;

    return (Node *)coldef;
}

/* Helper to create ColumnDef for ALTER COLUMN SET COLLATE only */
static Node *makeColumnDefForAlterCollateOnly(const char *colname, Node *collate_clause) {
    ColumnDef *coldef = makeColumnDef(0, 0);
    coldef->colname = tecc_pstrdup(colname);
    coldef->typeName = NULL; /* No type change, only collation */
    coldef->constraints = NIL;
    coldef->collClause = collate_clause;

    return (Node *)coldef;
}

#ifdef __cplusplus
}
#endif

/* 辅助结构：用于在 opt_partition_of 规则中同时传递父表和分区边界 */
typedef struct PartitionOfClause {
    Node   *parent;
    Node   *partbound;
} PartitionOfClause;

/* JOIN type constants */
#define JOIN_INNER 0
#define JOIN_LEFT 1
#define JOIN_RIGHT 2
#define JOIN_FULL 3
#define JOIN_CROSS 4
%}

%token <str> IDENT STRING
%token <str> INTEGER
%token <str> FLOAT
%token <str> SCONST BCONST XCONST ECONST
%token MAXVALUE MINVALUE
%token <ival> PARAM

/* SQL keywords */
%token SELECT INSERT UPDATE DELETE CREATE DROP ALTER
%token EXPLAIN ANALYZE VERBOSE
%token KEK DEK TABLE VIEW INDEX SEQUENCE FUNCTION
%token MATERIALIZED REPLACE REFRESH  /* ReqID:TECC4.0 - 密态视图支持 */
%token KEY_ALGO KEY_LENGTH KEY_PATH KEY_STORE KEY_TYPE
%token ARRAY
%token CURRENT_DATE CURRENT_TIME CURRENT_TIMESTAMP
%token WITH RECURSIVE WHERE FROM SET INTO VALUES
%token ADD COLUMN CONSTRAINT DATA TYPE ENCRYPTED
%token TEMPORARY TEMP COLLATE CHECK UNIQUE PRIMARY KEY REFERENCES
%token FOREIGN
%token AND OR NOT NULL_P IS BETWEEN LIKE ILIKE IN
%token REGEX_NOT_MATCH REGEX_NOT_MATCH_CI REGEX_MATCH REGEX_MATCH_CI
%token TRUE_P FALSE_P UNKNOWN
%token EXISTS ALL ANY SOME SYMMETRIC SIMILAR VARIADIC WITHIN
%token GROUP BY HAVING ORDER ASC DESC
%token LIMIT OFFSET DISTINCT
%token UNION INTERSECT EXCEPT
%token JOIN INNER LEFT RIGHT FULL OUTER ON
%token CASCADE RESTRICT
%token IF_P EXISTS_P
%token PREPARE EXECUTE DEALLOCATE
%token DECLARE CURSOR FOR FETCH MOVE CLOSE
%token COPY TO STDIN STDOUT BINARY CSV TEXT RENAME SCHEMA
%token FORMAT DELIMITER ENCODING FREEZE
%token HEADER QUOTE ESCAPE
%token FORCE_QUOTE FORCE_NOT_NULL FORCE_NULL
%token WINDOW USING AS RETURNING
%token DEFAULT MATCH PARTIAL SIMPLE NO ACTION INHERITS INHERIT
/* Additional tokens for CREATE TABLE and constraint support */
%token UNLOGGED OF TABLESPACE PARTITION RANGE LIST HASH
%token DEFERRABLE INITIALLY DEFERRED IMMEDIATE
%token GENERATED IDENTITY ALWAYS STORED EXCLUDE
%token COMMIT PRESERVE ROWS INCLUDING EXCLUDING

/* SELECT增强新增token */
%token CROSS NATURAL CASE WHEN THEN ELSE_P END_P OVER CAST
%token ATTACH DETACH

/* RowExpr / ROW constructor */
%token ROW

/* INSERT ON CONFLICT新增token */
%token DO NOTHING CONFLICT
%token FILTER
%token NULLS_P FIRST_P LAST_P
%token LATERAL
/* ReqID:TECC4.0 - DML语法补完: 锁定子句 + OVERRIDING */
%token SHARE NOWAIT SKIP LOCKED OVERRIDING USER_P SYSTEM_P VALUE_P
%token AT_P TIME_P ZONE_P

/* ReqID:TECC4.0 - SET 语句语法关键字（完整 SET 变体支持） */
%token LOCAL SESSION TRANSACTION CHARACTERISTICS
%token CATALOG_P NAMES ROLE AUTHORIZATION
%token XML_P OPTION DOCUMENT CONTENT SNAPSHOT
%token ISOLATION LEVEL READ UNCOMMITTED COMMITTED
%token REPEATABLE SERIALIZABLE ONLY WRITE CURRENT_P

/* Operators */
%token PLUS MINUS STAR SLASH PERCENT
%token EQUAL NOT_EQUAL LESS GREATER LESS_EQUAL GREATER_EQUAL
%token CONCAT
%token TYPECAST
%token OPERATOR OP /* ReqID:TECC4.0 - OPERATOR() syntax + dummy precedence token */

/* Punctuation */
%token SEMICOLON COMMA DOT LPAREN RPAREN LBRACKET RBRACKET
%token COLON DOLLAR

/* Expression precedence */
%nonassoc IS NULL_P
%left OR
%left AND
%right NOT
/* ReqID:TECC4.0 - IN/BETWEEN 必须高于 AND/OR（与 PostgreSQL 一致），否则
 * `id=1 AND a IN (1,2)` 会被解析成 `(id=1 AND a) IN (1,2)`，加密列 a 被
 * 包在 BoolExpr 里，比较识别为 expr=expr，明文字面量不加密。 */
%nonassoc IN BETWEEN LIKE ILIKE SIMILAR REGEX_NOT_MATCH REGEX_NOT_MATCH_CI REGEX_MATCH REGEX_MATCH_CI
%nonassoc EQUAL NOT_EQUAL LESS GREATER LESS_EQUAL GREATER_EQUAL
%left OP /* ReqID:TECC4.0 - OPERATOR() generic precedence */
%left PLUS MINUS CONCAT
%left '^'
%left STAR SLASH PERCENT
%left COLLATE
%left TYPECAST
%nonassoc UMINUS
%nonassoc FILTER OVER
/* ReqID:TECC4.0 - BooleanTest must bind tighter than AND/OR.
 * Using a separate precedence token to avoid redeclaration of IS. */
%nonassoc IS_TRUE_PREC
%nonassoc AT_P

%type <node> select_stmt insert_stmt update_stmt delete_stmt relation_expr_opt_alias
%type <node> expr b_expr simple_expr
%type <node> input stmt
%type <list> stmt_list
%type <node> create_kek_stmt alter_kek_stmt create_dek_stmt drop_stmt create_table_stmt alter_table_stmt
%type <ival> opt_drop_behavior
%type <node> kek_param dek_param
%type <node> PreparableStmt prepare_stmt execute_stmt deallocate_stmt declare_stmt copy_stmt create_as_stmt view_stmt matview_stmt
%type <list> prep_type_clause execute_param_clause
%type <list> opt_column_list opt_on_conflict opt_using
%type <list> opt_order
%type <node> column_def table_constraint constraint_spec constraint_def
%type <list> table_element_list alter_table_cmds column_constraint_list
%type <node> alter_table_cmd
%type <node> type_name
%type <str> col_name
%type <ival> iconst
%type <list> any_name any_name_list attrs
%type <node> opt_limit opt_offset
%type <list> set_clause
%type <node> insert_rest values_clause
%type <list> set_clause_list set_target_list
%type <node> column_ref param_ref constant
%type <list> kek_opt_list dek_opt_list expr_list type_list
%type <list> opt_all_clause
%type <node> func_call index_elem func_expr_list
%type <list> index_list
%type <node> sortby
%type <list> sortby_list
%type <ival> opt_asc_desc opt_nulls_order

/* ReqID:TECC4.0 - SET 语句规则类型声明 */
%type <node> VariableSetStmt set_rest generic_set set_rest_more
%type <node> var_value zone_value transaction_mode_item document_or_content opt_interval
%type <list> var_list transaction_mode_list
%type <str> var_name opt_boolean_or_string iso_level opt_encoding ident_or_sconst

/* 添加缺失的类型声明 */
%type <list> kek_opt_options dek_opt_options
%type <list> copy_options copy_option_list copy_option_list_nonempty
%type <node> copy_option
%type <str> name col_id sconst stmt_name
%type <list> target_list column_list
%type <list> opt_from_clause from_list opt_returning
%type <node> qualified_name
%type <node> opt_where opt_having
%type <list> opt_group opt_window

/* EXPLAIN 语句类型声明 */
%type <node> explain_stmt explainable_stmt

/* WITH CTE 类型声明 */
%type <node> with_clause cte
%type <list> cte_list

/* SubLink 类型声明 */
%type <node> any_oper
%type <list> qual_Op any_operator /* ReqID:TECC4.0 */
%type <str> all_Op /* ReqID:TECC4.0 */

/* ReqID:TECC4.0 - Indirection 表达式类型声明 */
%type <node> indirection_el opt_slice_bound
%type <list> indirection

/* Window/aggregate 类型声明 */
%type <list> opt_within_group_clause
%type <node> opt_filter_clause opt_over_clause
%type <node> window_specification window_definition
%type <list> window_definition_list opt_partition_clause opt_sort_clause

/* 更多缺失的类型声明 */
%type <node> a_expr
%type <bval> opt_temp opt_with_data
%type <node> table_element
%type <node> opt_collate
%type <list> opt_inherit qualified_name_list
%type <node> reference_option
/* ReqID:TECC4.0 - DML语法补完 type声明 */
%type <list> opt_locking_clause for_locking_items opt_locked_rels opt_nowait_or_skip
%type <node> for_locking_item
%type <str> for_locking_strength
%type <ival> opt_overriding
%type <str> reference_action
%type <list> opt_reloptions
%type <list> reloption_list
%type <node> reloption_elem
%type <list> opt_on_commit
%type <list> opt_of_table_column_list
%type <list> of_table_element_list
%type <node> of_table_element
%type <node> of_column_def
%type <node> opt_tablespace
%type <list> opt_partition_by
%type <node> like_table
%type <bval> opt_like_options
%type <list> exclude_elem_list
%type <node> exclude_operator
%type <node> opt_partition_of for_values
%type <list> for_val_list
%type <node> for_val

/* SELECT增强新增类型声明 */
%type <node> simple_select table_ref basic_table_ref
%type <node> select_operand
%type <node> opt_table_alias
%type <node> case_expr when_clause
%type <list> when_clause_list

/* INSERT ON CONFLICT新增类型声明 */
%type <node> conflict_target conflict_action

/* 约束相关类型声明 */
%type <node> column_constraint
%type <list> opt_column_constraint_list

/* 所有缺失的类型声明 */
%type <node> column_elem
%type <list> opt_reference_options reference_options
%type <list> id_list

/* 加密类型相关声明 */
%type <list> encrypted_options
%type <node> encrypted_option

%%

input:
    stmt_list   {
        tecc_parsetree = $1;
        if (tecc_parsetree)
            tecc_parsetree->type = FE_T_List;
        /* Clear the semantic value to prevent Bison from freeing it */
        $1 = NULL;
    }
    | stmt_list SEMICOLON   {
        tecc_parsetree = $1;
        if (tecc_parsetree)
            tecc_parsetree->type = FE_T_List;
        /* Clear the semantic value to prevent Bison from freeing it */
        $1 = NULL;
    }
    ;

stmt_list:
    stmt                 {
        $$ = list_make1($1);
    }
    | stmt_list SEMICOLON stmt     {
        $$ = lappend($1, $3);
    }
    ;

stmt:
    create_kek_stmt      {
        $$ = $1;
    }
    | alter_kek_stmt      {
        $$ = $1;
    }
    | create_dek_stmt    {
        $$ = $1;
    }
    | create_table_stmt  {
        $$ = $1;
    }
    | alter_table_stmt   {
        $$ = $1;
    }
    | drop_stmt          {
        $$ = $1;
    }
    | select_stmt        {
        $$ = $1;
    }
    | insert_stmt        {
        $$ = $1;
    }
    | update_stmt        {
        $$ = $1;
    }
    | delete_stmt        {
        $$ = $1;
    }
    | prepare_stmt       {
        $$ = $1;
    }
    | execute_stmt       {
        $$ = $1;
    }
    | deallocate_stmt    {
        $$ = $1;
    }
    | declare_stmt       {
        $$ = $1;
    }
    | copy_stmt          {
        $$ = $1;
    }
    | create_as_stmt     {
        $$ = $1;
    }
    | view_stmt          {
        $$ = $1;
    }
    | matview_stmt       {
        $$ = $1;
    }
    | explain_stmt          {
        $$ = $1;
    }
    | with_clause select_stmt {
        /* ReqID:TECC4.0 - Fix type confusion: select_stmt may return
         * SetOperationStmt (for UNION/INTERSECT/EXCEPT), not SelectStmt.
         * Traversing to the leftmost SelectStmt avoids heap overflow. */
        Node *n = $2;
        if (n != NULL && nodeTag(n) == FE_T_SelectStmt) {
            ((SelectStmt *)n)->cteList = (List *)$1;
        } else if (n != NULL && nodeTag(n) == FE_T_SetOp) {
            SetOperationStmt *setop = (SetOperationStmt *)n;
            Node *left = setop->larg;
            while (left != NULL && nodeTag(left) == FE_T_SetOp) {
                left = ((SetOperationStmt *)left)->larg;
            }
            if (left != NULL && nodeTag(left) == FE_T_SelectStmt) {
                ((SelectStmt *)left)->cteList = (List *)$1;
            }
        }
        $$ = n;
    }
    | with_clause insert_stmt {
        InsertStmt *n = (InsertStmt *)$2;
        n->withClause = (List *)$1;
        $$ = (Node *)n;
    }
    | with_clause update_stmt {
        UpdateStmt *n = (UpdateStmt *)$2;
        n->withClause = (List *)$1;
        $$ = (Node *)n;
    }
    | with_clause delete_stmt {
        DeleteStmt *n = (DeleteStmt *)$2;
        n->withClause = (List *)$1;
        $$ = (Node *)n;
    }
    | VariableSetStmt    {
        $$ = $1;
    }
    ;

/* ReqID:TECC4.0 - SET 语句完整语法（对齐 PG 14 gram.y VariableSetStmt）。
 * 所有 SET 变体解析为 SetStmt 透传节点，TECC 处理器透传不重写。
 * 目的：多语句消息（如 "SET ...; SELECT 密态表达式"）能整体解析成功，
 * SELECT 的表达式结果列元数据（pending_expr_col）正常收集。
 * 扩展：SetStmt 携带 kind/args 取值信息，供处理器对 SET search_path
 * 做同串即时性预测（ParameterStatus 权威值到达前预写缓存）。 */
VariableSetStmt:
    SET set_rest                    {
        SetStmt *n = (SetStmt *) $2;
        n->is_local = false;
        $$ = (Node *) n;
    }
    | SET LOCAL set_rest            {
        SetStmt *n = (SetStmt *) $3;
        n->is_local = true;
        $$ = (Node *) n;
    }
    | SET SESSION set_rest          {
        SetStmt *n = (SetStmt *) $3;
        n->is_local = false;
        $$ = (Node *) n;
    }
    ;

set_rest:
    TRANSACTION transaction_mode_list {
        $$ = (Node *) makeSetStmt(NULL, false, @1.first_line, TECC_SET_MULTI, NIL);
    }
    | SESSION CHARACTERISTICS AS TRANSACTION transaction_mode_list {
        $$ = (Node *) makeSetStmt(NULL, false, @1.first_line, TECC_SET_MULTI, NIL);
    }
    | set_rest_more
    ;

generic_set:
    var_name TO var_list            {
        $$ = (Node *) makeSetStmt($1, false, @1.first_line, TECC_SET_VALUE, $3);
    }
    | var_name EQUAL var_list       {
        $$ = (Node *) makeSetStmt($1, false, @1.first_line, TECC_SET_VALUE, $3);
    }
    | var_name TO DEFAULT           {
        $$ = (Node *) makeSetStmt($1, false, @1.first_line, TECC_SET_DEFAULT, NIL);
    }
    | var_name EQUAL DEFAULT        {
        $$ = (Node *) makeSetStmt($1, false, @1.first_line, TECC_SET_DEFAULT, NIL);
    }
    ;

set_rest_more:
    generic_set                     { $$ = $1; }
    | var_name FROM CURRENT_P       {
        $$ = (Node *) makeSetStmt($1, false, @1.first_line, TECC_SET_CURRENT, NIL);
    }
    | TIME_P ZONE_P zone_value      {
        $$ = (Node *) makeSetStmt("timezone", false, @1.first_line, TECC_SET_MULTI, NIL);
    }
    | CATALOG_P STRING              {
        $$ = (Node *) makeSetStmt(NULL, false, @1.first_line, TECC_SET_MULTI, NIL);
    }
    | SCHEMA STRING                 {
        $$ = (Node *) makeSetStmt("search_path", false, @1.first_line, TECC_SET_VALUE,
                                  list_make1(makeStringConst($2, @2.first_line, 0)));
    }
    | NAMES opt_encoding            {
        $$ = (Node *) makeSetStmt("client_encoding", false, @1.first_line, TECC_SET_MULTI, NIL);
    }
    | ROLE ident_or_sconst          {
        $$ = (Node *) makeSetStmt("role", false, @1.first_line, TECC_SET_MULTI, NIL);
    }
    | SESSION AUTHORIZATION ident_or_sconst {
        $$ = (Node *) makeSetStmt("session_authorization", false, @1.first_line, TECC_SET_MULTI, NIL);
    }
    | SESSION AUTHORIZATION DEFAULT {
        $$ = (Node *) makeSetStmt("session_authorization", false, @1.first_line, TECC_SET_MULTI, NIL);
    }
    | XML_P OPTION document_or_content {
        $$ = (Node *) makeSetStmt("xmloption", false, @1.first_line, TECC_SET_MULTI, NIL);
    }
    | TRANSACTION SNAPSHOT STRING   {
        $$ = (Node *) makeSetStmt(NULL, false, @1.first_line, TECC_SET_MULTI, NIL);
    }
    ;

var_name:
    IDENT                           { $$ = $1; }
    | var_name DOT IDENT            { $$ = $1; }
    ;

var_list:
    var_value                       { $$ = list_make1($1); }
    | var_list ',' var_value      { $$ = lappend($1, $3); }
    ;

var_value:
    opt_boolean_or_string           { $$ = (Node *) makeStringConst($1, @1.first_line, 0); }
    | INTEGER                       { $$ = (Node *) makeStringConst($1, @1.first_line, 0); }
    | FLOAT                         { $$ = (Node *) makeStringConst($1, @1.first_line, 0); }
    | MINUS INTEGER                 { $$ = (Node *) makeStringConst($2, @1.first_line, 0); }
    | MINUS FLOAT                   { $$ = (Node *) makeStringConst($2, @1.first_line, 0); }
    ;

opt_boolean_or_string:
    TRUE_P                          { $$ = "true"; }
    | FALSE_P                       { $$ = "false"; }
    | ON                            { $$ = "on"; }
    | IDENT                         { $$ = $1; }
    | STRING                        { $$ = $1; }
    ;

zone_value:
    STRING                          { $$ = (Node *) makeStringConst($1, @1.first_line, 0); }
    | IDENT                         { $$ = (Node *) makeStringConst($1, @1.first_line, 0); }
    | INTEGER                       { $$ = (Node *) makeStringConst($1, @1.first_line, 0); }
    | FLOAT                         { $$ = (Node *) makeStringConst($1, @1.first_line, 0); }
    | MINUS INTEGER                 { $$ = (Node *) makeStringConst($2, @1.first_line, 0); }
    | MINUS FLOAT                   { $$ = (Node *) makeStringConst($2, @1.first_line, 0); }
    | DEFAULT                       { $$ = NULL; }
    | LOCAL                         { $$ = NULL; }
    /* ReqID:TECC4.0 - SET TIME ZONE INTERVAL '...' [HOUR TO MINUTE] 等：
     * INTERVAL 在前端是 IDENT（非保留字），opt_interval 用 IDENT/IDENT TO IDENT
     * 覆盖 YEAR/HOUR/DAY 等全部区间字段（均为 IDENT）。 */
    | IDENT STRING opt_interval     { $$ = (Node *) makeStringConst($2, @1.first_line, 0); }
    ;

opt_interval:
    /* EMPTY */                     { $$ = NULL; }
    | IDENT                         { $$ = NULL; }
    | IDENT TO IDENT                { $$ = NULL; }
    ;

opt_encoding:
    STRING                          { $$ = $1; }
    | DEFAULT                       { $$ = NULL; }
    | /* EMPTY */                   { $$ = NULL; }
    ;

transaction_mode_list:
    transaction_mode_item           { $$ = list_make1($1); }
    | transaction_mode_list ',' transaction_mode_item { $$ = lappend($1, $3); }
    | transaction_mode_list transaction_mode_item      { $$ = lappend($1, $2); }
    ;

transaction_mode_item:
    ISOLATION LEVEL iso_level       { $$ = (Node *) makeStringConst($3, @1.first_line, 0); }
    | READ ONLY                     { $$ = (Node *) makeStringConst("read only", @1.first_line, 0); }
    | READ WRITE                    { $$ = (Node *) makeStringConst("read write", @1.first_line, 0); }
    | DEFERRABLE                    { $$ = (Node *) makeStringConst("deferrable", @1.first_line, 0); }
    | NOT DEFERRABLE                { $$ = (Node *) makeStringConst("not deferrable", @1.first_line, 0); }
    ;

iso_level:
    READ UNCOMMITTED                { $$ = "read uncommitted"; }
    | READ COMMITTED                { $$ = "read committed"; }
    | REPEATABLE READ               { $$ = "repeatable read"; }
    | SERIALIZABLE                  { $$ = "serializable"; }
    ;

document_or_content:
    DOCUMENT                        { $$ = (Node *) makeStringConst("DOCUMENT", @1.first_line, 0); }
    | CONTENT                       { $$ = (Node *) makeStringConst("CONTENT", @1.first_line, 0); }
    ;

ident_or_sconst:
    IDENT                           { $$ = $1; }
    | STRING                        { $$ = $1; }
    ;

opt_reloptions:
    /* empty */        { $$ = NIL; }
    | WITH '(' reloption_list ')'  { $$ = $3; }
    ;

reloption_list:
    reloption_elem                     { $$ = list_make1($1); }
    | reloption_list ',' reloption_elem { $$ = lappend($1, $3); }
    ;

reloption_elem:
    name                               { $$ = (Node *)makeDefElem($1, NULL, @1.first_line); }
    | name EQUAL IDENT                 { $$ = (Node *)makeDefElem($1, (Node *)makeStringConst($3, @1.first_line, 0), @1.first_line); }
    | name EQUAL STRING                { $$ = (Node *)makeDefElem($1, (Node *)makeStringConst($3, @1.first_line, 0), @1.first_line); }
    | name EQUAL INTEGER               { $$ = (Node *)makeDefElem($1, (Node *)makeStringConst($3, @1.first_line, 0), @1.first_line); }
    | name EQUAL FLOAT                 { $$ = (Node *)makeDefElem($1, (Node *)makeStringConst($3, @1.first_line, 0), @1.first_line); }
    ;

/* CREATE KEK statement */
create_kek_stmt:
    CREATE KEK any_name kek_opt_options
    {
        CreateKekStmt *n = makeCreateKekStmt(@1.first_line, @$.last_column - @1.first_column + 1);
        n->names = $3;
        n->options = $4;
        n->if_not_exists = false;
        $$ = (Node *)n;
    }
    | CREATE KEK IF_P NOT EXISTS any_name kek_opt_options
    {
        CreateKekStmt *n = makeCreateKekStmt(@1.first_line, @$.last_column - @1.first_column + 1);
        n->names = $6;
        n->options = $7;
        n->if_not_exists = true;
        $$ = (Node *)n;
    }
    ;

kek_opt_options:
    WITH '(' kek_opt_list ')'  { $$ = $3; }
    ;

/* ALTER KEK statement */
alter_kek_stmt:
    ALTER KEK any_name kek_opt_options
    {
        AlterKekStmt *n = makeAlterKekStmt(@1.first_line, @$.last_column - @1.first_column + 1);
        n->names = $3;
        n->options = $4;
        $$ = (Node *)n;
    }
    ;

/* CREATE DEK statement */
create_dek_stmt:
    CREATE DEK any_name dek_opt_options
    {
        CreateDekStmt *n = makeCreateDekStmt(@1.first_line, @$.last_column - @1.first_column + 1);
        n->names = $3;
        n->options = $4;
        n->if_not_exists = false;
        $$ = (Node *)n;
    }
    | CREATE DEK IF_P NOT EXISTS any_name dek_opt_options
    {
        CreateDekStmt *n = makeCreateDekStmt(@1.first_line, @$.last_column - @1.first_column + 1);
        n->names = $6;
        n->options = $7;
        n->if_not_exists = true;
        $$ = (Node *)n;
    }
    ;

dek_opt_options:
    WITH VALUES '(' dek_opt_list ')'  { $$ = $4; }
    ;

/* CREATE TABLE statement */
create_table_stmt:
    CREATE opt_temp TABLE qualified_name '(' table_element_list ')' opt_inherit opt_partition_of opt_partition_by opt_reloptions opt_on_commit opt_tablespace
    {
        CreateStmt *n = makeCreateStmt(@1.first_line, @$.last_column - @1.first_column + 1);
        n->relation = (RangeVar *)$4;
        n->tableElts = $6;
        n->options = $11;
        n->inhRelations = $8;
        if ($9 != NULL) {
            PartitionOfClause *clause = (PartitionOfClause *)$9;
            n->inhRelations = list_make1(clause->parent);
            n->partbound = clause->partbound;
        }
        n->partspec = $10;
        n->ofTypename = NULL;
        n->is_temp = $2;
        $$ = (Node *)n;
    }
    | CREATE opt_temp TABLE IF_P NOT EXISTS qualified_name '(' table_element_list ')' opt_inherit opt_partition_of opt_partition_by opt_reloptions opt_on_commit opt_tablespace
    {
        CreateStmt *n = makeCreateStmt(@1.first_line, @$.last_column - @1.first_column + 1);
        n->relation = (RangeVar *)$7;
        n->tableElts = $9;
        n->options = $14;
        n->inhRelations = $11;
        if ($12 != NULL) {
            PartitionOfClause *clause = (PartitionOfClause *)$12;
            n->inhRelations = list_make1(clause->parent);
            n->partbound = clause->partbound;
        }
        n->partspec = $13;
        n->ofTypename = NULL;
        n->is_temp = $2;
        $$ = (Node *)n;
    }
    | CREATE opt_temp TABLE qualified_name OF any_name opt_of_table_column_list opt_reloptions opt_on_commit opt_tablespace
    {
        CreateStmt *n = makeCreateStmt(@1.first_line, @$.last_column - @1.first_column + 1);
        n->relation = (RangeVar *)$4;
        n->tableElts = $7;
        n->options = $8;
        n->inhRelations = NIL;
        n->partbound = NULL;
        n->partspec = NULL;
        n->ofTypename = makeTypeNameFromList($6, @6.first_line, @6.last_column - @6.first_column + 1);
        n->is_temp = $2;
        $$ = (Node *)n;
    }
    | CREATE opt_temp TABLE IF_P NOT EXISTS qualified_name OF any_name opt_of_table_column_list opt_reloptions opt_on_commit opt_tablespace
    {
        CreateStmt *n = makeCreateStmt(@1.first_line, @$.last_column - @1.first_column + 1);
        n->relation = (RangeVar *)$7;
        n->tableElts = $10;
        n->options = $11;
        n->inhRelations = NIL;
        n->partbound = NULL;
        n->partspec = NULL;
        n->ofTypename = makeTypeNameFromList($9, @9.first_line, @9.last_column - @9.first_column + 1);
        n->is_temp = $2;
        $$ = (Node *)n;
    }
    | CREATE opt_temp TABLE qualified_name PARTITION OF qualified_name for_values opt_partition_by opt_reloptions opt_on_commit opt_tablespace
    {
        CreateStmt *n = makeCreateStmt(@1.first_line, @$.last_column - @1.first_column + 1);
        n->relation = (RangeVar *)$4;
        n->tableElts = NIL;
        n->options = $10;
        n->inhRelations = list_make1($7);
        n->partbound = $8;
        n->partspec = $9;
        n->ofTypename = NULL;
        n->is_temp = $2;
        $$ = (Node *)n;
    }
    | CREATE opt_temp TABLE IF_P NOT EXISTS qualified_name PARTITION OF qualified_name for_values opt_partition_by opt_reloptions opt_on_commit opt_tablespace
    {
        CreateStmt *n = makeCreateStmt(@1.first_line, @$.last_column - @1.first_column + 1);
        n->relation = (RangeVar *)$7;
        n->tableElts = NIL;
        n->options = $13;
        n->inhRelations = list_make1($10);
        n->partbound = $11;
        n->partspec = $12;
        n->ofTypename = NULL;
        n->is_temp = $2;
        $$ = (Node *)n;
    }
    ;

/* ALTER TABLE statement */
alter_table_stmt:
    ALTER TABLE qualified_name alter_table_cmds
    {
        AlterTableStmt *n = makeAlterTableStmt(@1.first_line, @$.last_column - @1.first_column + 1);
        n->relation = (RangeVar *)$3;
        n->cmds = $4;
        $$ = (Node *)n;
    }
    ;

alter_table_cmds:
    alter_table_cmd                     { $$ = list_make1($1); }
    | alter_table_cmds ',' alter_table_cmd { $$ = lappend($1, $3); }
    ;

alter_table_cmd:
    ALTER COLUMN col_name SET DATA TYPE type_name opt_collate
                                        { $$ = (Node *)makeAlterTableCmd(AT_AlterColumnType, makeColumnDefForAlter($3, $7, $8), $3, DROP_RESTRICT, false, @1.first_line); }
    | ALTER COLUMN col_name TYPE type_name opt_collate
                                        { $$ = (Node *)makeAlterTableCmd(AT_AlterColumnType, makeColumnDefForAlter($3, $5, $6), $3, DROP_RESTRICT, false, @1.first_line); }
    | ALTER COLUMN col_name SET COLLATE any_name
                                        { $$ = (Node *)makeAlterTableCmd(AT_AlterColumnType, makeColumnDefForAlterCollateOnly($3, $6), $3, DROP_RESTRICT, false, @1.first_line); }
    | ALTER COLUMN col_name SET DEFAULT expr
                                        { $$ = (Node *)makeAlterTableCmd(AT_GenericOptions, $6, $3, DROP_RESTRICT, false, @1.first_line); }
    | ALTER COLUMN col_name DROP DEFAULT
                                        { $$ = (Node *)makeAlterTableCmd(AT_ResetOptions, NULL, $3, DROP_RESTRICT, false, @1.first_line); }
    | ALTER COLUMN col_name SET NOT NULL_P
                                        { $$ = (Node *)makeAlterTableCmd(AT_SetNotNull, NULL, $3, DROP_RESTRICT, false, @1.first_line); }
    | ALTER COLUMN col_name DROP NOT NULL_P
                                        { $$ = (Node *)makeAlterTableCmd(AT_DropNotNull, NULL, $3, DROP_RESTRICT, false, @1.first_line); }
    | ADD COLUMN column_def               { $$ = (Node *)makeAlterTableCmd(AT_AddColumn, $3, NULL, DROP_RESTRICT, false, @1.first_line); }
    | ADD COLUMN IF_P NOT EXISTS column_def
                                        { $$ = (Node *)makeAlterTableCmd(AT_AddColumn, $6, NULL, DROP_RESTRICT, true, @1.first_line); }
    | DROP COLUMN col_name              { $$ = (Node *)makeAlterTableCmd(AT_DropColumn, NULL, $3, DROP_RESTRICT, false, @1.first_line); }
    | DROP COLUMN IF_P EXISTS col_name  { $$ = (Node *)makeAlterTableCmd(AT_DropColumn, NULL, $5, DROP_RESTRICT, true, @1.first_line); }
    | DROP COLUMN col_name CASCADE      { $$ = (Node *)makeAlterTableCmd(AT_DropColumn, NULL, $3, DROP_CASCADE, false, @1.first_line); }
    | DROP COLUMN col_name RESTRICT     { $$ = (Node *)makeAlterTableCmd(AT_DropColumn, NULL, $3, DROP_RESTRICT, false, @1.first_line); }
    | DROP COLUMN IF_P EXISTS col_name CASCADE
                                        { $$ = (Node *)makeAlterTableCmd(AT_DropColumn, NULL, $5, DROP_CASCADE, true, @1.first_line); }
    | DROP COLUMN IF_P EXISTS col_name RESTRICT
                                        { $$ = (Node *)makeAlterTableCmd(AT_DropColumn, NULL, $5, DROP_RESTRICT, true, @1.first_line); }
    | ADD CONSTRAINT name constraint_def
                                        { $$ = (Node *)makeAlterTableCmd(AT_AddConstraint, $4, $3, DROP_RESTRICT, false, @1.first_line); }
    | DROP CONSTRAINT name              { $$ = (Node *)makeAlterTableCmd(AT_DropConstraint, NULL, $3, DROP_RESTRICT, false, @1.first_line); }
    | RENAME COLUMN col_name TO name    { $$ = (Node *)makeAlterTableCmd(AT_GenericOptions, (Node *)makeString($5), $3, DROP_RESTRICT, false, @1.first_line); }
    | RENAME TO name                    { $$ = (Node *)makeAlterTableCmd(AT_GenericOptions, (Node *)makeString($3), NULL, DROP_RESTRICT, false, @1.first_line); }
    | SET SCHEMA name                   { $$ = (Node *)makeAlterTableCmd(AT_GenericOptions, (Node *)makeString($3), NULL, DROP_RESTRICT, false, @1.first_line); }
    | ATTACH PARTITION qualified_name for_values
                                        { $$ = (Node *)makeAlterTableCmd(AT_AttachPartition, $4, NULL, DROP_RESTRICT, false, @1.first_line); }
    | DETACH PARTITION qualified_name   { $$ = (Node *)makeAlterTableCmd(AT_DetachPartition, NULL, $3, DROP_RESTRICT, false, @1.first_line); }
    ;


opt_temp:
    /* empty */        { $$ = false; }
    | TEMPORARY        { $$ = true; }
    | TEMP             { $$ = true; }
    | UNLOGGED         { $$ = false; }
    ;

table_element_list:
    table_element                    { $$ = list_make1($1); }
    | table_element_list ',' table_element { $$ = lappend($1, $3); }
    ;

table_element:
    column_def                       { $$ = $1; }
    | table_constraint               { $$ = $1; }
    | like_table                     { $$ = $1; }
    ;

like_table:
    LIKE qualified_name opt_like_options
    {
        $$ = (Node *)makeDefElem("like", $2, @1.first_line);
    }
    ;

opt_like_options:
    /* empty */                      { $$ = 0; }
    | INCLUDING name                 { $$ = 0; }
    | EXCLUDING name                 { $$ = 0; }
    | INCLUDING ALL                  { $$ = 0; }
    | EXCLUDING ALL                  { $$ = 0; }
    ;

column_def:
    col_name type_name opt_collate opt_column_constraint_list
    {
        ColumnDef *n = makeColumnDef(@1.first_line, @$.last_column - @1.first_column + 1);
        n->colname = tecc_pstrdup($1);
        n->typeName = $2;
        n->collClause = $3;
        n->constraints = $4;
        $$ = (Node *)n;
    }
    ;

col_name:
    name                             { $$ = $1; }
    ;

type_name:
    name                             { $$ = makeTypeName($1, @1.first_line, @1.last_column - @1.first_column + 1); }
    /* ReqID:TECC4.0 - Qualified type name: schema.type */
    | name attrs                     { $$ = makeTypeNameFromList(lcons(makeString($1), $2), @1.first_line, @$.last_column - @1.first_column + 1); }
    | name '(' iconst ')'            { $$ = makeTypeNameWithTypmod($1, $3, @1.first_line, @$.last_column - @1.first_column + 1); }
    | name '(' iconst ',' iconst ')' { $$ = makeTypeNameWithTypmod2($1, $3, $5, @1.first_line, @$.last_column - @1.first_column + 1); }
    | name ENCRYPTED WITH '(' encrypted_options ')' { $$ = makeEncryptedTypeName($1, $5, @1.first_line, @$.last_column - @1.first_column + 1); }
    | name '(' iconst ')' ENCRYPTED WITH '(' encrypted_options ')' { $$ = makeEncryptedTypeNameWithTypmod($1, $3, $8, @1.first_line, @$.last_column - @1.first_column + 1); }
    | name '(' iconst ',' iconst ')' ENCRYPTED WITH '(' encrypted_options ')' { $$ = makeEncryptedTypeNameWithTypmod2($1, $3, $5, $10, @1.first_line, @$.last_column - @1.first_column + 1); }
    ;

encrypted_options:
    encrypted_option                  { $$ = list_make1($1); }
    | encrypted_options ',' encrypted_option { $$ = lappend($1, $3); }
    ;

encrypted_option:
    name EQUAL name                    { $$ = makeDefElem($1, (Node *)makeString($3), @2.first_line); }
    | name EQUAL STRING                { $$ = makeDefElem($1, (Node *)makeString($3), @2.first_line); }

opt_collate:
    /* empty */                       { $$ = NULL; }
    | COLLATE any_name                { $$ = (Node *)makeCollateClause(NULL, $2, @1.first_line); }
    ;

opt_column_constraint_list:
    /* empty */                       { $$ = NIL; }
    | column_constraint_list          { $$ = $1; }
    ;

column_constraint_list:
    column_constraint                 { $$ = list_make1($1); }
    | column_constraint_list column_constraint { $$ = lappend($1, $2); }
    ;

column_constraint:
    NULL_P                            { $$ = makeConstraint(CONSTR_NULL, @1.first_line, @1.last_column - @1.first_column + 1); }
    | NOT NULL_P                      { $$ = makeConstraint(CONSTR_NOTNULL, @1.first_line, @2.last_column - @1.first_column + 1); }
    | CHECK '(' a_expr ')'            { $$ = makeConstraintWithExpr(CONSTR_CHECK, $3, @1.first_line, @$.last_column - @1.first_column + 1); }
    | CHECK '(' a_expr ')' NO INHERIT { $$ = makeConstraintWithExpr(CONSTR_CHECK, $3, @1.first_line, @$.last_column - @1.first_column + 1); }
    | DEFAULT a_expr                  { $$ = makeConstraintWithExpr(CONSTR_DEFAULT, $2, @1.first_line, @$.last_column - @1.first_column + 1); }
    | UNIQUE                          { $$ = makeConstraint(CONSTR_UNIQUE, @1.first_line, @1.last_column - @1.first_column + 1); }
    | PRIMARY KEY                     { $$ = makeConstraint(CONSTR_PRIMARY, @1.first_line, @2.last_column - @1.first_column + 1); }
    | REFERENCES qualified_name '(' column_list ')' opt_reference_options
                                     { $$ = makeForeignKeyConstraint($2, $4, $6, @1.first_line, @$.last_column - @1.first_column + 1); }
    | DEFERRABLE                      { $$ = makeConstraint(CONSTR_ATTR_DEFERRABLE, @1.first_line, @1.last_column - @1.first_column + 1); }
    | INITIALLY DEFERRED              { $$ = makeConstraint(CONSTR_ATTR_DEFERRED, @1.first_line, @2.last_column - @1.first_column + 1); }
    | INITIALLY IMMEDIATE             { $$ = makeConstraint(CONSTR_ATTR_IMMEDIATE, @1.first_line, @2.last_column - @1.first_column + 1); }
    | GENERATED ALWAYS AS '(' a_expr ')' STORED
                                     { $$ = makeConstraintWithExpr(CONSTR_GENERATED, $5, @1.first_line, @$.last_column - @1.first_column + 1); }
    | GENERATED ALWAYS AS IDENTITY    { $$ = makeConstraint(CONSTR_IDENTITY, @1.first_line, @3.last_column - @1.first_column + 1); }
    | GENERATED BY DEFAULT AS IDENTITY { $$ = makeConstraint(CONSTR_IDENTITY, @1.first_line, @4.last_column - @1.first_column + 1); }
    ;

table_constraint:
    CONSTRAINT name constraint_spec   { $$ = setConstraintName($3, $2, @2.first_line, @2.last_column - @2.first_column + 1); }
    | constraint_spec                 { $$ = $1; }
    ;

constraint_spec:
    CHECK '(' a_expr ')'              { $$ = makeConstraintWithExpr(CONSTR_CHECK, $3, @1.first_line, @$.last_column - @1.first_column + 1); }
    | CHECK '(' a_expr ')' NO INHERIT { $$ = makeConstraintWithExpr(CONSTR_CHECK, $3, @1.first_line, @$.last_column - @1.first_column + 1); }
    | UNIQUE '(' index_list ')'      { $$ = makeConstraintWithColumns(CONSTR_UNIQUE, $3, @1.first_line, @$.last_column - @1.first_column + 1); }
    | PRIMARY KEY '(' column_list ')' { $$ = makeConstraintWithColumns(CONSTR_PRIMARY, $4, @1.first_line, @$.last_column - @1.first_column + 1); }
    | FOREIGN KEY '(' column_list ')' REFERENCES qualified_name '(' column_list ')' opt_reference_options
                                     { $$ = makeForeignKeyConstraint($7, $4, $9, @1.first_line, @$.last_column - @1.first_column + 1); }
    | EXCLUDE USING name '(' exclude_elem_list ')' opt_where
                                     { $$ = makeConstraintWithColumns(CONSTR_EXCLUSION, $5, @1.first_line, @$.last_column - @1.first_column + 1); }
    ;

exclude_elem_list:
    column_elem WITH exclude_operator  { $$ = list_make1($1); }
    | exclude_elem_list ',' column_elem WITH exclude_operator { $$ = lappend($1, $3); }
    ;

exclude_operator:
    name              { $$ = (Node *)makeString($1); }
    | EQUAL           { $$ = (Node *)makeString("="); }
    | NOT_EQUAL       { $$ = (Node *)makeString("<>"); }
    | LESS            { $$ = (Node *)makeString("<"); }
    | GREATER         { $$ = (Node *)makeString(">"); }
    | LESS_EQUAL      { $$ = (Node *)makeString("<="); }
    | GREATER_EQUAL   { $$ = (Node *)makeString(">="); }
    ;

/* Constraint definition for ALTER TABLE */
constraint_def:
    constraint_spec                   { $$ = $1; }
    ;


column_list:
    column_elem                       { $$ = list_make1($1); }
    | column_list ',' column_elem     { $$ = lappend($1, $3); }
    ;

column_elem:
    col_name                          { $$ = (Node *)makeStringConst($1, @1.first_line, 0); }
    ;

/* index_list: list of index elements (expressions or column names) for UNIQUE constraints */
index_list:
    index_elem                        { $$ = list_make1($1); }
    | index_list ',' index_elem       { $$ = lappend($1, $3); }
    ;

index_elem:
    b_expr                            { $$ = $1; }
    ;

opt_reference_options:
    /* empty */                       { $$ = NIL; }
    | reference_options               { $$ = $1; }
    ;

reference_options:
    reference_option                  { $$ = list_make1($1); }
    | reference_options reference_option { $$ = lappend($1, $2); }
    ;

reference_option:
    MATCH FULL                        { $$ = (Node *)makeDefElem("match", (Node *)makeString("full"), @1.first_line); }
    | MATCH PARTIAL                   { $$ = (Node *)makeDefElem("match", (Node *)makeString("partial"), @1.first_line); }
    | MATCH SIMPLE                    { $$ = (Node *)makeDefElem("match", (Node *)makeString("simple"), @1.first_line); }
    | ON DELETE reference_action      { $$ = (Node *)makeDefElem("on_delete", (Node *)makeString($3), @1.first_line); }
    | ON UPDATE reference_action      { $$ = (Node *)makeDefElem("on_update", (Node *)makeString($3), @1.first_line); }
    ;

reference_action:
    NO ACTION                         { $$ = "no action"; }
    | RESTRICT                        { $$ = "restrict"; }
    | CASCADE                         { $$ = "cascade"; }
    | SET NULL_P                      { $$ = "set null"; }
    | SET DEFAULT                     { $$ = "set default"; }
    ;

opt_inherit:
    /* empty */                       { $$ = NIL; }
    | INHERITS '(' qualified_name_list ')' { $$ = $3; }
    ;

opt_partition_by:
    /* empty */                       { $$ = NIL; }
    | PARTITION BY RANGE '(' column_list ')' { $$ = $5; }
    | PARTITION BY LIST '(' column_list ')'  { $$ = $5; }
    | PARTITION BY HASH '(' column_list ')'  { $$ = $5; }
    ;


opt_partition_of:
    /* empty */                       { $$ = NULL; }
    | PARTITION OF qualified_name for_values
        {
            PartitionOfClause *clause = (PartitionOfClause *)tecc_palloc(sizeof(PartitionOfClause));
            clause->parent = $3;
            clause->partbound = $4;
            $$ = (Node *)clause;
        }
    ;

for_values:
    FOR VALUES FROM '(' for_val_list ')' TO '(' for_val_list ')'
        {
            PartitionBoundSpec *spec = makePartitionBoundSpec('r', false, @1.first_line);
            spec->lowerdatums = $5;
            spec->upperdatums = $9;
            $$ = (Node *)spec;
        }
    | FOR VALUES IN '(' for_val_list ')'
        {
            PartitionBoundSpec *spec = makePartitionBoundSpec('l', false, @1.first_line);
            spec->listdatums = $5;
            $$ = (Node *)spec;
        }
    | FOR VALUES DEFAULT
        {
            PartitionBoundSpec *spec = makePartitionBoundSpec(0, true, @1.first_line);
            $$ = (Node *)spec;
        }
    ;

for_val_list:
    for_val                            { $$ = list_make1($1); }
    | for_val_list ',' for_val         { $$ = lappend($1, $3); }
    ;

for_val:
    STRING
        {
            $$ = (Node *)makePartitionRangeDatum(makeStringConst($1, @1.first_line, @1.last_column), @1.first_line);
        }
    | INTEGER
        {
            int int_value = atoi($1);
            $$ = (Node *)makePartitionRangeDatum((Node *)makeIntConst(int_value, @1.first_line, @1.last_column), @1.first_line);
        }
    | INTEGER TYPECAST name
        {
            /* Handle "10000::int2" — ignore the type cast, keep INTEGER value */
            int int_value = atoi($1);
            $$ = (Node *)makePartitionRangeDatum((Node *)makeIntConst(int_value, @1.first_line, @1.last_column), @1.first_line);
        }
    | MINUS INTEGER
        {
            /* Handle "-32767" — combine minus + integer into a single negative Const */
            int int_value = -atoi($2);
            int full_location = @1.first_line;
            int full_length = @1.last_column + @2.last_column;
            $$ = (Node *)makePartitionRangeDatum((Node *)makeIntConst(int_value, full_location, full_length), @1.first_line);
        }
    | MINUS INTEGER TYPECAST name
        {
            /* Handle "-32767::int2" — combine minus + integer, ignore the type cast */
            int int_value = -atoi($2);
            int full_location = @1.first_line;
            int full_length = @1.last_column + @2.last_column;
            $$ = (Node *)makePartitionRangeDatum((Node *)makeIntConst(int_value, full_location, full_length), @1.first_line);
        }
    | FLOAT
        {
            double float_value = strtod($1, NULL);
            $$ = (Node *)makePartitionRangeDatum((Node *)makeFloatConst(float_value, @1.first_line, @1.last_column), @1.first_line);
        }
    | FLOAT TYPECAST name
        {
            /* Handle "3.14::float4" — ignore the type cast, keep FLOAT value */
            double float_value = strtod($1, NULL);
            $$ = (Node *)makePartitionRangeDatum((Node *)makeFloatConst(float_value, @1.first_line, @1.last_column), @1.first_line);
        }
    | MINUS FLOAT
        {
            /* Handle "-20.0" — combine minus + float */
            double float_value = -strtod($2, NULL);
            int full_length = @1.last_column + @2.last_column;
            $$ = (Node *)makePartitionRangeDatum((Node *)makeFloatConst(float_value, @1.first_line, full_length), @1.first_line);
        }
    | MINUS FLOAT TYPECAST name
        {
            /* Handle "-99999999.99::numeric" — combine minus + float, ignore type cast */
            double float_value = -strtod($2, NULL);
            int full_length = @1.last_column + @2.last_column;
            $$ = (Node *)makePartitionRangeDatum((Node *)makeFloatConst(float_value, @1.first_line, full_length), @1.first_line);
        }
    | IDENT
        {
            /* Handle 'Infinity' as a special float value in partition bounds */
            $$ = (Node *)makePartitionRangeDatum(
                (Node *)makeStringConst($1, @1.first_line, @1.last_column), @1.first_line);
        }
    | MINUS IDENT
        {
            /* Handle "-Infinity" — combine minus + ident for special float values */
            size_t slen = strlen($2);
            char *neg = (char *)pg_malloc(slen + 2);
            int full_length = @1.last_column + @2.last_column;
            neg[0] = '-'; memcpy(neg + 1, $2, slen + 1);
            $$ = (Node *)makePartitionRangeDatum(
                (Node *)makeStringConst(neg, @1.first_line, full_length), @1.first_line);
            pg_free(neg);
        }
    | MAXVALUE
        {
            $$ = (Node *)makePartitionRangeDatum(NULL, @1.first_line);
        }
    | MINVALUE
        {
            $$ = (Node *)makePartitionRangeDatum(NULL, @1.first_line);
        }
    ;
opt_of_table_column_list:
    /* empty */                       { $$ = NIL; }
    | '(' of_table_element_list ')'   { $$ = $2; }
    ;

of_table_element_list:
    of_table_element                    { $$ = list_make1($1); }
    | of_table_element_list ',' of_table_element { $$ = lappend($1, $3); }
    ;

of_table_element:
    of_column_def                       { $$ = $1; }
    | table_constraint                  { $$ = $1; }
    ;

of_column_def:
    col_name opt_of_column_options opt_column_constraint_list
    {
        ColumnDef *n = makeColumnDef(@1.first_line, @$.last_column - @1.first_column + 1);
        n->colname = tecc_pstrdup($1);
        n->typeName = NULL;
        n->constraints = $3;
        $$ = (Node *)n;
    }
    ;

opt_of_column_options:
    WITH IDENT    { /* WITH OPTIONS - processed by backend */ }
    | /* empty */
    ;

opt_on_commit:
    /* empty */                       { $$ = NIL; }
    | ON COMMIT PRESERVE ROWS        { $$ = NIL; }
    | ON COMMIT DELETE ROWS          { $$ = NIL; }
    | ON COMMIT DROP                 { $$ = NIL; }
    ;

opt_tablespace:
    /* empty */                       { $$ = NULL; }
    | TABLESPACE name                 { $$ = (Node *)makeStringConst($2, @1.first_line, 0); }
    ;

qualified_name_list:
    qualified_name                    { $$ = list_make1($1); }
    | qualified_name_list ',' qualified_name { $$ = lappend($1, $3); }
    ;

/* DROP statement */
drop_stmt:
    DROP KEK name opt_drop_behavior
    {
        DropStmt *n = makeDropStmt(@1.first_line, @$.last_column - @1.first_column + 1);
        n->objects = list_make1(makeString($3));
        n->removeType = OBJECT_KEK;
        n->behavior = $4;
        n->missing_ok = false;
        $$ = (Node *)n;
    }
    | DROP DEK name opt_drop_behavior
    {
        DropStmt *n = makeDropStmt(@1.first_line, @$.last_column - @1.first_column + 1);
        n->objects = list_make1(makeString($3));
        n->removeType = OBJECT_DEK;
        n->behavior = $4;
        n->missing_ok = false;
        $$ = (Node *)n;
    }
    | DROP TABLE qualified_name opt_drop_behavior
    {
        DropStmt *n = makeDropStmt(@1.first_line, @$.last_column - @1.first_column + 1);
        /* $3 is a RangeVar*, need to extract the table name */
        RangeVar *rv = (RangeVar *)$3;
        n->objects = list_make1(makeStringConst(rv->relname, @3.first_line, 0));
        n->removeType = OBJECT_TABLE;
        n->behavior = $4;
        n->missing_ok = false;
        $$ = (Node *)n;
    }
    | DROP KEK IF_P EXISTS name opt_drop_behavior
    {
        DropStmt *n = makeDropStmt(@1.first_line, @$.last_column - @1.first_column + 1);
        n->objects = list_make1(makeString($5));
        n->removeType = OBJECT_KEK;
        n->behavior = $6;
        n->missing_ok = true;
        $$ = (Node *)n;
    }
    | DROP DEK IF_P EXISTS name opt_drop_behavior
    {
        DropStmt *n = makeDropStmt(@1.first_line, @$.last_column - @1.first_column + 1);
        n->objects = list_make1(makeString($5));
        n->removeType = OBJECT_DEK;
        n->behavior = $6;
        n->missing_ok = true;
        $$ = (Node *)n;
    }
    | DROP TABLE IF_P EXISTS qualified_name opt_drop_behavior
    {
        DropStmt *n = makeDropStmt(@1.first_line, @$.last_column - @1.first_column + 1);
        /* $5 is a RangeVar*, need to extract the table name */
        RangeVar *rv = (RangeVar *)$5;
        n->objects = list_make1(makeString(rv->relname));
        n->removeType = OBJECT_TABLE;
        n->behavior = $6;
        n->missing_ok = true;
        $$ = (Node *)n;
    }
    ;

/* ReqID:TECC4.0 - Bug 9326 - 集合运算的操作数。
 * 对齐 PG14 的 select_clause/select_with_parens：操作数可以是普通 SELECT，
 * 也可以是带括号的（嵌套）集合表达式。缺这条规则时 `(SELECT ...) UNION/
 * INTERSECT/EXCEPT ...` 与 `SELECT ... EXCEPT (SELECT ...)` 在前端整体解析
 * 失败（syntax error, unexpected '('），语句被掩码后原文下发，其中的密态
 * 常量不会加密，服务端报 invalid hex string。
 * 括号只作分组，不引入新节点：'(' select_stmt ')' 直接返回内层节点。 */
select_operand:
    simple_select                        { $$ = $1; }
    | '(' select_stmt ')'                { $$ = $2; }
    ;

/* SELECT statement with UNION/INTERSECT/EXCEPT support */
select_stmt:
    select_operand                       { $$ = $1; }
    | select_stmt UNION ALL select_operand
    {
        $$ = (Node *)makeSetOperationStmt(SETOP_UNION, true, $1, $4, @2.first_line, @$.last_column - @2.first_column + 1);
    }
    | select_stmt UNION select_operand
    {
        $$ = (Node *)makeSetOperationStmt(SETOP_UNION, false, $1, $3, @2.first_line, @$.last_column - @2.first_column + 1);
    }
    | select_stmt INTERSECT ALL select_operand
    {
        $$ = (Node *)makeSetOperationStmt(SETOP_INTERSECT, true, $1, $4, @2.first_line, @$.last_column - @2.first_column + 1);
    }
    | select_stmt INTERSECT select_operand
    {
        $$ = (Node *)makeSetOperationStmt(SETOP_INTERSECT, false, $1, $3, @2.first_line, @$.last_column - @2.first_column + 1);
    }
    | select_stmt EXCEPT ALL select_operand
    {
        $$ = (Node *)makeSetOperationStmt(SETOP_EXCEPT, true, $1, $4, @2.first_line, @$.last_column - @2.first_column + 1);
    }
    | select_stmt EXCEPT select_operand
    {
        $$ = (Node *)makeSetOperationStmt(SETOP_EXCEPT, false, $1, $3, @2.first_line, @$.last_column - @2.first_column + 1);
    }
    ;

/* Simple SELECT without UNION */
simple_select:
    SELECT opt_all_clause target_list opt_from_clause opt_where opt_group opt_having opt_window opt_order opt_limit opt_offset
        opt_locking_clause
    {
        SelectStmt *n = makeSelectStmt(@1.first_line, @$.last_column - @1.first_column + 1);
        n->distinctClause = $2;
        n->targetList = $3;
        n->fromClause = $4;
        n->whereClause = $5;
        n->groupClause = $6;
        n->havingClause = $7;
        n->windowClause = $8;
        n->sortClause = $9;
        n->limitCount = $10;
        n->limitOffset = $11;
        n->lockingClause = $12;
        $$ = (Node *)n;
    }
    | values_clause
    {
        /* ReqID:TECC4.0 - VALUES as simple_select variant, aligned with PG14 backend */
        $$ = $1;
    }
    ;

/* INSERT statement */
insert_stmt:
    INSERT INTO qualified_name opt_column_list opt_overriding insert_rest opt_on_conflict opt_returning
    {
        InsertStmt *n = makeInsertStmt(@1.first_line, @$.last_column - @1.first_column + 1);
        n->relation = (struct RangeVar *)$3;
        n->cols = $4;
        n->override = $5;
        n->selectStmt = $6;
        n->onConflictClause = $7;
        n->returningList = $8;
        $$ = (Node *)n;
    }
    ;

/* ReqID:TECC4.0 - UPDATE statement with relation_expr_opt_alias */
update_stmt:
    UPDATE relation_expr_opt_alias SET set_clause_list opt_from_clause opt_where opt_returning
    {
        UpdateStmt *n = makeUpdateStmt(@1.first_line, @$.last_column - @1.first_column + 1);
        n->relation = (struct RangeVar *)$2;
        n->targetList = $4;
        n->fromClause = $5;
        n->whereClause = $6;
        n->returningList = $7;
        $$ = (Node *)n;
    }
    ;

/* ReqID:TECC4.0 - DELETE statement with relation_expr_opt_alias */
delete_stmt:
    DELETE FROM relation_expr_opt_alias opt_using opt_where opt_returning
    {
        DeleteStmt *n = makeDeleteStmt(@1.first_line, @$.last_column - @1.first_column + 1);
        n->relation = (struct RangeVar *)$3;
        n->usingClause = $4;
        n->whereClause = $5;
        n->returningList = $6;
        $$ = (Node *)n;
    }
    ;


/* ReqID:TECC4.0 - qual_Op: qualified operator name for OPERATOR() syntax */
all_Op:
    PLUS                                    { $$ = "+"; }
    | MINUS                                 { $$ = "-"; }
    | STAR                                  { $$ = "*"; }
    | SLASH                                 { $$ = "/"; }
    | PERCENT                               { $$ = "%"; }
    | '^'                                   { $$ = "^"; }
    | EQUAL                                 { $$ = "="; }
    | NOT_EQUAL                             { $$ = "<>"; }
    | LESS                                  { $$ = "<"; }
    | GREATER                               { $$ = ">"; }
    | LESS_EQUAL                            { $$ = "<="; }
    | GREATER_EQUAL                         { $$ = ">="; }
    | CONCAT                                { $$ = "||"; }
    | REGEX_MATCH                           { $$ = "~"; }
    | REGEX_MATCH_CI                        { $$ = "~*"; }
    | REGEX_NOT_MATCH                       { $$ = "!~"; }
    | REGEX_NOT_MATCH_CI                    { $$ = "!~*"; }
    ;

any_operator:
    IDENT                                   { $$ = list_make1(makeString($1)); }
    | all_Op                                { $$ = list_make1(makeString($1)); }
    | IDENT '.' any_operator                { $$ = lcons(makeString($1), $3); }
    ;

qual_Op:
    OPERATOR '(' any_operator ')'           { $$ = $3; }
    ;

/* Expressions - b_expr excludes boolean operators (AND, OR, NOT) */
b_expr:
    simple_expr          { $$ = $1; }
    | b_expr PLUS b_expr     { $$ = make_a_expr(AEXPR_OP, list_make1(makeString("+")), $1, $3, @2.first_line); }
    | b_expr MINUS b_expr    { $$ = make_a_expr(AEXPR_OP, list_make1(makeString("-")), $1, $3, @2.first_line); }
    | b_expr '^' b_expr          { $$ = make_a_expr(AEXPR_OP, list_make1(makeString("^")), $1, $3, @2.first_line); }
    | b_expr STAR b_expr     { $$ = make_a_expr(AEXPR_OP, list_make1(makeString("*")), $1, $3, @2.first_line); }
    | b_expr SLASH b_expr    { $$ = make_a_expr(AEXPR_OP, list_make1(makeString("/")), $1, $3, @2.first_line); }
    | b_expr PERCENT b_expr  { $$ = make_a_expr(AEXPR_OP, list_make1(makeString("%")), $1, $3, @2.first_line); }
    | b_expr EQUAL b_expr    { $$ = make_a_expr(AEXPR_OP, list_make1(makeString("=")), $1, $3, @2.first_line); }
    | b_expr NOT_EQUAL b_expr { $$ = make_a_expr(AEXPR_OP, list_make1(makeString("<>")), $1, $3, @2.first_line); }
    | b_expr LESS b_expr     { $$ = make_a_expr(AEXPR_OP, list_make1(makeString("<")), $1, $3, @2.first_line); }
    | b_expr GREATER b_expr  { $$ = make_a_expr(AEXPR_OP, list_make1(makeString(">")), $1, $3, @2.first_line); }
    | b_expr LESS_EQUAL b_expr { $$ = make_a_expr(AEXPR_OP, list_make1(makeString("<=")), $1, $3, @2.first_line); }
    | b_expr GREATER_EQUAL b_expr { $$ = make_a_expr(AEXPR_OP, list_make1(makeString(">=")), $1, $3, @2.first_line); }
    | b_expr IS NULL_P     { $$ = make_null_test($1, IS_NULL, @2.first_line); }
    | b_expr CONCAT b_expr   { $$ = make_a_expr(AEXPR_OP, list_make1(makeString("||")), $1, $3, @2.first_line); }
    | b_expr IS NOT NULL_P { $$ = make_null_test($1, IS_NOT_NULL, @2.first_line); }
    | b_expr IS DISTINCT FROM b_expr %prec IS
    {
        $$ = make_a_expr(AEXPR_OP, list_make1(makeString("IS DISTINCT FROM")), $1, $5, @2.first_line);
    }
    | b_expr IS NOT DISTINCT FROM b_expr %prec IS
    {
        $$ = make_a_expr(AEXPR_OP, list_make1(makeString("IS NOT DISTINCT FROM")), $1, $6, @2.first_line);
    }
    /* ReqID:TECC4.0 - BooleanTest: IS TRUE/FALSE/UNKNOWN */
    | b_expr IS TRUE_P %prec IS_TRUE_PREC
    {
        $$ = (Node *)makeBooleanTest($1, IS_TRUE, @2.first_line);
    }
    | b_expr IS NOT TRUE_P %prec IS_TRUE_PREC
    {
        $$ = (Node *)makeBooleanTest($1, IS_NOT_TRUE, @2.first_line);
    }
    | b_expr IS FALSE_P %prec IS_TRUE_PREC
    {
        $$ = (Node *)makeBooleanTest($1, IS_FALSE, @2.first_line);
    }
    | b_expr IS NOT FALSE_P %prec IS_TRUE_PREC
    {
        $$ = (Node *)makeBooleanTest($1, IS_NOT_FALSE, @2.first_line);
    }
    | b_expr IS UNKNOWN %prec IS_TRUE_PREC
    {
        $$ = (Node *)makeBooleanTest($1, IS_UNKNOWN, @2.first_line);
    }
    | b_expr IS NOT UNKNOWN %prec IS_TRUE_PREC
    {
        $$ = (Node *)makeBooleanTest($1, IS_NOT_UNKNOWN, @2.first_line);
    }
    /* ReqID:TECC4.0 - AT TIME ZONE expression */
    | b_expr AT_P TIME_P ZONE_P b_expr %prec AT_P
    {
        $$ = (Node *)makeTimeZoneExpr($1, $5, @2.first_line);
    }
    | b_expr any_oper ANY '(' select_stmt ')'
    {
        $$ = (Node *)makeSubLink(ANY_SUBLINK, $1, $2, $5, @2.first_line);
    }
    | b_expr any_oper SOME '(' select_stmt ')'
    {
        $$ = (Node *)makeSubLink(ANY_SUBLINK, $1, $2, $5, @2.first_line);
    }
    | b_expr any_oper ALL '(' select_stmt ')'
    {
        $$ = (Node *)makeSubLink(ALL_SUBLINK, $1, $2, $5, @2.first_line);
    }
    /* ReqID:TECC4.0 - ANY/SOME/ALL over an ARRAY literal, e.g. `a = ANY(ARRAY[1,2])`.
     * PostgreSQL's generic subquery_Op form; expand into OR/AND of element
     * comparisons so the constant-encryption path handles each element.
     * Without this the statement fails to parse and is sent unencrypted. */
    | b_expr any_oper ANY '(' ARRAY '[' expr_list ']' ')'
    {
        $$ = make_any_all_array_expr($2, $1, (List *) $7, 1, @2.first_line);
    }
    | b_expr any_oper SOME '(' ARRAY '[' expr_list ']' ')'
    {
        $$ = make_any_all_array_expr($2, $1, (List *) $7, 1, @2.first_line);
    }
    | b_expr any_oper ALL '(' ARRAY '[' expr_list ']' ')'
    {
        $$ = make_any_all_array_expr($2, $1, (List *) $7, 0, @2.first_line);
    }
    /* ReqID:TECC4.0 - qual_Op: OPERATOR(schema.opname) syntax */
    | b_expr qual_Op b_expr %prec OP
    {
        $$ = make_a_expr(AEXPR_OP, $2, $1, $3, @2.first_line);
    }
    ;

/* Full expression including boolean operators */
expr:
    b_expr                { $$ = $1; }
    | expr AND expr      { $$ = make_bool_expr(AND_EXPR, list_make2($1, $3), @2.first_line); }
    | expr OR expr       { $$ = make_bool_expr(OR_EXPR, list_make2($1, $3), @2.first_line); }
    | NOT expr %prec NOT  { $$ = make_bool_expr(NOT_EXPR, list_make1($2), @1.first_line); }
    | expr BETWEEN SYMMETRIC b_expr AND b_expr %prec BETWEEN {
        /* BETWEEN SYMMETRIC: order-independent, i.e. (a>=x AND a<=y) OR (a>=y AND a<=x) */
        Node *lower1 = make_a_expr(AEXPR_OP, list_make1(makeString(">=")), $1, $4, @2.first_line);
        Node *upper1 = make_a_expr(AEXPR_OP, list_make1(makeString("<=")), $1, $6, @4.first_line);
        Node *range1 = make_bool_expr(AND_EXPR, list_make2(lower1, upper1), @2.first_line);
        Node *lower2 = make_a_expr(AEXPR_OP, list_make1(makeString(">=")), $1, $6, @2.first_line);
        Node *upper2 = make_a_expr(AEXPR_OP, list_make1(makeString("<=")), $1, $4, @4.first_line);
        Node *range2 = make_bool_expr(AND_EXPR, list_make2(lower2, upper2), @2.first_line);
        $$ = make_bool_expr(OR_EXPR, list_make2(range1, range2), @2.first_line);
      }
    | expr BETWEEN b_expr AND b_expr %prec BETWEEN {
        /* BETWEEN a AND b is equivalent to a >= x AND a <= y */
        Node *lower = make_a_expr(AEXPR_OP, list_make1(makeString(">=")), $1, $3, @2.first_line);
        Node *upper = make_a_expr(AEXPR_OP, list_make1(makeString("<=")), $1, $5, @4.first_line);
        $$ = make_bool_expr(AND_EXPR, list_make2(lower, upper), @2.first_line);
      }
    | expr IN '(' expr_list ')' {
        /* IN (x, y, z) is equivalent to expr = x OR expr = y OR expr = z */
        List *args = NIL;
        List *expr_list = (List *)$4;
        ListCell *lc;
        /* 使用标准C循环替换foreach宏 */
        for (lc = list_head(expr_list); lc != NULL; lc = lnext(lc)) {
            Node *arg = (Node *)lfirst(lc);
            Node *eq = make_a_expr(AEXPR_OP, list_make1(makeString("=")), $1, arg, @2.first_line);
            args = lappend(args, eq);
        }
        if (list_length(args) == 1) {
            $$ = (Node *)lfirst(list_head(args));
        } else {
            $$ = make_bool_expr(OR_EXPR, args, @2.first_line);
        }
      }
    | expr LIKE expr
        { $$ = make_a_expr_escape(AEXPR_LIKE, list_make1(makeString("LIKE")), $1, $3, NULL, @2.first_line); }
    | expr LIKE expr ESCAPE a_expr
        { $$ = make_a_expr_escape(AEXPR_LIKE, list_make1(makeString("LIKE")), $1, $3, $5, @2.first_line); }
    | expr NOT LIKE expr %prec NOT
        { $$ = make_a_expr_escape(AEXPR_LIKE, list_make1(makeString("NOT LIKE")), $1, $4, NULL, @2.first_line); }
    | expr NOT LIKE expr ESCAPE a_expr %prec NOT
        { $$ = make_a_expr_escape(AEXPR_LIKE, list_make1(makeString("NOT LIKE")), $1, $4, $6, @2.first_line); }
    | expr ILIKE expr
        { $$ = make_a_expr_escape(AEXPR_LIKE, list_make1(makeString("ILIKE")), $1, $3, NULL, @2.first_line); }
    | expr ILIKE expr ESCAPE a_expr
        { $$ = make_a_expr_escape(AEXPR_LIKE, list_make1(makeString("ILIKE")), $1, $3, $5, @2.first_line); }
    | expr NOT ILIKE expr %prec NOT
        { $$ = make_a_expr_escape(AEXPR_LIKE, list_make1(makeString("NOT ILIKE")), $1, $4, NULL, @2.first_line); }
    | expr NOT ILIKE expr ESCAPE a_expr %prec NOT
        { $$ = make_a_expr_escape(AEXPR_LIKE, list_make1(makeString("NOT ILIKE")), $1, $4, $6, @2.first_line); }
    | expr SIMILAR TO expr
        { $$ = make_a_expr_escape(AEXPR_SIMILAR, list_make1(makeString("SIMILAR TO")), $1, $4, NULL, @2.first_line); }
    | expr SIMILAR TO expr ESCAPE a_expr
        { $$ = make_a_expr_escape(AEXPR_SIMILAR, list_make1(makeString("SIMILAR TO")), $1, $4, $6, @2.first_line); }
    | expr NOT SIMILAR TO expr %prec NOT
        { $$ = make_a_expr_escape(AEXPR_SIMILAR, list_make1(makeString("NOT SIMILAR TO")), $1, $5, NULL, @2.first_line); }
    | expr NOT SIMILAR TO expr ESCAPE a_expr %prec NOT
        { $$ = make_a_expr_escape(AEXPR_SIMILAR, list_make1(makeString("NOT SIMILAR TO")), $1, $5, $7, @2.first_line); }
    | expr REGEX_NOT_MATCH expr
        { $$ = make_a_expr(AEXPR_OP, list_make1(makeString("!~")), $1, $3, @2.first_line); }
    | expr REGEX_NOT_MATCH_CI expr
        { $$ = make_a_expr(AEXPR_OP, list_make1(makeString("!~*")), $1, $3, @2.first_line); }
    | expr REGEX_MATCH expr
        { $$ = make_a_expr(AEXPR_OP, list_make1(makeString("~")), $1, $3, @2.first_line); }
    | expr REGEX_MATCH_CI expr
        { $$ = make_a_expr(AEXPR_OP, list_make1(makeString("~*")), $1, $3, @2.first_line); }
    | expr NOT BETWEEN SYMMETRIC b_expr AND b_expr %prec NOT {
        /* NOT BETWEEN SYMMETRIC: order-independent, i.e. (a<x OR a>y) AND (a<y OR a>x) */
        Node *lower1 = make_a_expr(AEXPR_OP, list_make1(makeString("<")), $1, $5, @2.first_line);
        Node *upper1 = make_a_expr(AEXPR_OP, list_make1(makeString(">")), $1, $7, @4.first_line);
        Node *range1 = make_bool_expr(OR_EXPR, list_make2(lower1, upper1), @2.first_line);
        Node *lower2 = make_a_expr(AEXPR_OP, list_make1(makeString("<")), $1, $7, @2.first_line);
        Node *upper2 = make_a_expr(AEXPR_OP, list_make1(makeString(">")), $1, $5, @4.first_line);
        Node *range2 = make_bool_expr(OR_EXPR, list_make2(lower2, upper2), @2.first_line);
        $$ = make_bool_expr(AND_EXPR, list_make2(range1, range2), @2.first_line);
      }
    | expr NOT BETWEEN b_expr AND b_expr %prec NOT {
        Node *lower = make_a_expr(AEXPR_OP, list_make1(makeString("<")), $1, $4, @2.first_line);
        Node *upper = make_a_expr(AEXPR_OP, list_make1(makeString(">")), $1, $6, @4.first_line);
        $$ = make_bool_expr(OR_EXPR, list_make2(lower, upper), @2.first_line);
      }
    | expr NOT IN '(' expr_list ')' %prec NOT {
        List *args = NIL;
        List *elist = (List *)$5;
        ListCell *lc;
        for (lc = list_head(elist); lc != NULL; lc = lnext(lc)) {
            Node *arg = (Node *)lfirst(lc);
            Node *neq = make_a_expr(AEXPR_OP, list_make1(makeString("<>")), $1, arg, @2.first_line);
            args = lappend(args, neq);
        }
        if (list_length(args) == 1) {
            $$ = (Node *)lfirst(list_head(args));
        } else {
            $$ = make_bool_expr(AND_EXPR, args, @2.first_line);
        }
      }
    | expr IN '(' select_stmt ')'
    {
        $$ = (Node *)makeSubLink(ANY_SUBLINK, $1, list_make1(makeString("=")), $4, @2.first_line);
    }
    | expr NOT IN '(' select_stmt ')'
    {
        $$ = (Node *)makeSubLink(ALL_SUBLINK, $1, list_make1(makeString("<>")), $5, @2.first_line);
    }
    ;

a_expr:
    b_expr               { $$ = $1; }
    ;

simple_expr:
    column_ref          { $$ = $1; }
    | constant          { $$ = $1; }
    | param_ref         { $$ = $1; }
    /* ReqID:TECC4.0 - DEFAULT as expression value (INSERT VALUES, UPDATE SET) */
    | DEFAULT
    {
        $$ = (Node *)makeSetToDefault(@1.first_line);
    }
    | func_call         { $$ = $1; }
    | case_expr         { $$ = $1; }
    /* ReqID:TECC4.0 - Support parenthesized expressions at any level
     * (LIKE, AND, OR, etc.), not just b_expr. */
    | '(' expr ')'         { $$ = $2; }
    | EXISTS '(' select_stmt ')'
    {
        $$ = (Node *)makeSubLink(EXISTS_SUBLINK, NULL, NIL, $3, @1.first_line);
    }
    | ARRAY '[' expr_list ']'
    {
        $$ = (Node *)makeFuncCall(list_make1(makeStringConst("array", @1.first_line, 0)), $3, @1.first_line);
    }
    /* ReqID:TECC4.0 - ROW constructor for multi-column SET */
    | ROW '(' expr_list ')'
    {
        $$ = (Node *)makeRowExpr($3, @1.first_line);
    }
    /* ReqID:TECC4.0 - Implicit row constructor: (a, b) at least 2 elements.
     * Uses expr_list ',' b_expr to ensure >= 2 elements, avoiding conflict
     * with the single-expression '(' b_expr ')' rule above. */
    | '(' expr_list ',' b_expr ')'
    {
        List *args = lappend($2, $4);
        $$ = (Node *)makeRowExpr(args, @1.first_line);
    }
    | CURRENT_DATE
    {
        $$ = (Node *)makeFuncCall(list_make1(makeStringConst("current_date", @1.first_line, 0)), NIL, @1.first_line);
    }
    | CURRENT_TIME
    {
        $$ = (Node *)makeFuncCall(list_make1(makeStringConst("current_time", @1.first_line, 0)), NIL, @1.first_line);
    }
    | CURRENT_TIMESTAMP
    {
        $$ = (Node *)makeFuncCall(list_make1(makeStringConst("current_timestamp", @1.first_line, 0)), NIL, @1.first_line);
    }
    /* ReqID:TECC4.0 - USER is equivalent to current_user */
    | USER_P
    {
        $$ = (Node *)makeFuncCall(list_make1(makeStringConst("user", @1.first_line, 0)), NIL, @1.first_line);
    }
    /* ReqID:TECC4.0 - func_name Sconst: type-as-function syntax
     * e.g. TIMESTAMP '2024-01-01', DATE '2024-01-01' */
    | any_name_list STRING
    {
        $$ = (Node *)makeFuncCall($1, list_make1(makeString($2)), @1.first_line);
    }
    | '(' select_stmt ')'
    {
        $$ = (Node *)makeSubLink(EXPR_SUBLINK, NULL, NIL, $2, @1.first_line);
    }
    | PLUS simple_expr %prec UMINUS { $$ = make_a_expr(AEXPR_OP, list_make1(makeString("+")), NULL, $2, @1.first_line); }
    | MINUS simple_expr %prec UMINUS { $$ = make_a_expr(AEXPR_OP, list_make1(makeString("-")), NULL, $2, @1.first_line); }
    | simple_expr TYPECAST type_name
    {
        TypeCast *tc = (TypeCast *)tecc_palloc(sizeof(TypeCast));
        tc->type = FE_T_TypeCast;
        tc->arg = $1;
        tc->typeName = (TypeName *)$3;
        tc->location = @1.first_line;
        $$ = (Node *)tc;
    }
    /* ReqID:TECC4.0 - COLLATE expression: expr COLLATE "collation_name" */
    | simple_expr COLLATE any_name
    {
        $$ = (Node *)makeCollateClause($1, $3, @2.first_line);
    }
    ;

column_ref:
    any_name_list
    {
        $$ = make_column_ref_from_list($1, @1.first_line);
    }
    /* ReqID:TECC4.0 - column_ref with indirection: tab.col[idx] or tab.col[idx].field */
    | any_name_list indirection
    {
        Node *cr = make_column_ref_from_list($1, @1.first_line);
        $$ = (Node *)makeA_Indirection(cr, $2);
    }
    /* ReqID:TECC4.0 - Bug 907: 限定星号 tab.* / schema.tab.*
     * （后端 gram.y 里属 indirection_el: '.' '*'）。本语法的 '.' 形式统一
     * 交给 any_name_list，而 any_name_list 只吃标识符，故 tab.* 原先整体
     * 解析失败 —— 后果是该语句的 TECC 处理被静默跳过（不做 SQL 改写、
     * 不解密，密态列原样输出密文）。这里补一条产生式（放在 column_ref 上
     * 与后端一致），产出与后端同形的 ColumnRef{fields=[..., A_Star]}；
     * 实测不新增任何冲突。 */
    | any_name_list '.' STAR
    {
        $$ = (Node *)makeColumnRef(lappend($1, (Node *)makeA_Star()),
                                   @1.first_line);
    }
    ;

func_call:
	/* Standard function call */
	any_name_list '(' func_expr_list ')' opt_within_group_clause opt_filter_clause opt_over_clause
	{
		if ($3 != NULL && ((Node*)$3)->type == FE_T_FuncCall) {
			/* Aggregation star or distinct case (may include ORDER BY) */
			FuncCall *fc = (FuncCall*)$3;
			fc->funcname = $1;
			if ($5 != NIL) { fc->agg_order = $5; fc->agg_within_group = true; }
			if ($6 != NULL) fc->agg_filter = $6;
			if ($7 != NULL) fc->over = $7;
			$$ = (Node*)fc;
		} else {
			FuncCall *fc = makeFuncCall($1, (List*)$3, @1.first_line);
            fc->end_location = @4.first_line + 1; /* ReqID:TECC4.0 */
			if ($5 != NIL) { fc->agg_order = $5; fc->agg_within_group = true; }
			if ($6 != NULL) fc->agg_filter = $6;
			if ($7 != NULL) fc->over = $7;
			$$ = (Node*)fc;
		}
	}
	/* VARIADIC function call: func(VARIADIC array_expr) */
	| any_name_list '(' VARIADIC b_expr ')' opt_within_group_clause opt_filter_clause opt_over_clause
	{
		FuncCall *fc = makeFuncCall($1, list_make1($4), @1.first_line);
		fc->func_variadic = true;
		if ($6 != NIL) { fc->agg_order = $6; fc->agg_within_group = true; }
		if ($7 != NULL) fc->agg_filter = $7;
		if ($8 != NULL) fc->over = $8;
		$$ = (Node*)fc;
	}
	/* Mixed args + VARIADIC: func(a, b, VARIADIC arr) */
	| any_name_list '(' expr_list ',' VARIADIC b_expr ')' opt_within_group_clause opt_filter_clause opt_over_clause
	{
		FuncCall *fc = makeFuncCall($1, lappend($3, $6), @1.first_line);
		fc->func_variadic = true;
		if ($8 != NIL) { fc->agg_order = $8; fc->agg_within_group = true; }
		if ($9 != NULL) fc->agg_filter = $9;
		if ($10 != NULL) fc->over = $10;
		$$ = (Node*)fc;
	}
	;

func_expr_list:
    STAR    {
        FuncCall *fc = makeFuncCall(NIL, NIL, @1.first_line);
        fc->agg_star = true;
        $$ = (Node*)fc;
    }
    | /* empty */ {
        $$ = NULL;
    }
    | DISTINCT expr_list {
        FuncCall *fc = makeFuncCall(NIL, (List*)$2, @1.first_line);
        fc->agg_distinct = true;
        $$ = (Node*)fc;
    }
    | ALL expr_list {
        $$ = (Node*)$2;
    }
    | expr_list  { $$ = (Node*)$1; }
    | expr_list ORDER BY sortby_list {
        FuncCall *fc = makeFuncCall(NIL, (List*)$1, @1.first_line);
        fc->agg_order = $4;
        $$ = (Node*)fc;
    }
    | DISTINCT expr_list ORDER BY sortby_list {
        FuncCall *fc = makeFuncCall(NIL, (List*)$2, @1.first_line);
        fc->agg_distinct = true;
        fc->agg_order = $5;
        $$ = (Node*)fc;
    }
    ;

/* Aggregate ORDER BY clause (within window_specification only) */
opt_sort_clause:
    /* empty */                { $$ = NIL; }
    | ORDER BY sortby_list     { $$ = $3; }
    ;

/* WITHIN GROUP clause for ordered-set aggregates */
opt_within_group_clause:
    WITHIN GROUP '(' ORDER BY sortby_list ')'
    {
        $$ = $6;
    }
    | /* empty */
    {
        $$ = NIL;
    }
    ;

/* FILTER clause for aggregate functions */
opt_filter_clause:
    /* empty */ %prec UMINUS       { $$ = NULL; }
    | FILTER '(' WHERE expr ')'    { $$ = $4; }
    ;

/* OVER clause for window functions */
opt_over_clause:
    /* empty */ %prec UMINUS       { $$ = NULL; }
    | OVER window_specification    { $$ = $2; }
    ;

/* Window specification for OVER and WINDOW clauses */
window_specification:
    IDENT
    {
        $$ = (Node *)makeWindowDef(NULL, $1, NIL, NIL, @1.first_line);
    }
    | '(' opt_partition_clause opt_sort_clause ')'
    {
        $$ = (Node *)makeWindowDef(NULL, NULL, $2, $3, @1.first_line);
    }
    ;

opt_partition_clause:
    /* empty */                   { $$ = NIL; }
    | PARTITION BY expr_list      { $$ = $3; }
    ;

/* Window definition list for WINDOW clause */
window_definition_list:
    window_definition                                 { $$ = list_make1($1); }
    | window_definition_list ',' window_definition    { $$ = lappend($1, $3); }
    ;

window_definition:
    IDENT AS window_specification
    {
        WindowDef *wd = (WindowDef *)$3;
        wd->name = $1;
        $$ = (Node *)wd;
    }
    ;
constant:
    INTEGER            {
        char *endptr;
        errno = 0;
        (void)strtoll($1, &endptr, 10);
        if (errno == ERANGE)
        {
            /* Overflow: preserve exact text as string constant,
             * avoiding precision loss through int64 or float8.
             * A_Expr unary minus handler will prepend '-' for negation. */
            $$ = makeStringConst($1, @1.first_line, @1.last_column);
        }
        else
            $$ = makeIntConst(atoll($1), @1.first_line, @1.last_column);
    }
    | FLOAT            {
        /* ReqID:TECC4.0 - Preserve original text as string to avoid precision
         * loss from double conversion (strtod limits to ~15 significant digits).
         * The formatter handles string→numeric conversion with full precision
         * for all types (float4/8 via Ryu, numeric via digit-by-digit parsing).
         * Pre-requisite for Bug8178: DET equality match for high-precision
         * numeric values (>15 digits) between INSERT and WHERE paths.
         * 注意：用 makeNumericTextConst 而非 makeStringConst —— 同样保留文本，
         * 但为该常量打上数值形态标记，使 GREATEST/LEAST 等参数组合校验按数值族
         * 归类（否则 numeric/float 列 + 小数常量会被误判为"常量类型不匹配"）。 */
        $$ = makeNumericTextConst($1, @1.first_line, @1.last_column);
    }
    | STRING           {
        $$ = makeStringConst($1, @1.first_line, @1.last_column);
    }
    | ECONST           {
        $$ = makeStringConst($1, @1.first_line, @1.last_column);
    }
    | NULL_P           {
        $$ = makeNullConst(@1.first_line, @1.last_column);
    }
    | TRUE_P           {
        $$ = makeStringConst("true", @1.first_line, 4);
    }
    | FALSE_P          {
        $$ = makeStringConst("false", @1.first_line, 5);
    }
    ;

param_ref:
    PARAM              { $$ = (Node *)makeParam(PARAM_EXTERN, $1, 0, @1.first_line); }
    ;

/* CASE expression */
case_expr:
    CASE expr when_clause_list ELSE_P expr END_P
    {
        $$ = (Node *)makeCaseExpr($2, $5, $3, @1.first_line);
    }
    | CASE when_clause_list ELSE_P expr END_P
    {
        $$ = (Node *)makeCaseExpr(NULL, $4, $2, @1.first_line);
    }
    | CASE expr when_clause_list END_P
    {
        $$ = (Node *)makeCaseExpr($2, NULL, $3, @1.first_line);
    }
    | CASE when_clause_list END_P
    {
        $$ = (Node *)makeCaseExpr(NULL, NULL, $2, @1.first_line);
    }
    ;

when_clause_list:
    when_clause                            { $$ = list_make1($1); }
    | when_clause_list when_clause         { $$ = lappend($1, $2); }
    ;

when_clause:
    WHEN expr THEN expr
    {
        $$ = (Node *)makeCaseWhen($2, $4, @1.first_line);
    }
    ;

/* ReqID:TECC4.0 - Indirection 表达式语法规则
 * 仅支持数组下标 ([idx], [lidx:uidx])。
 * 字段选择 (.field, .*) 通过 any_name_list 点分机制覆盖，
 * 加入 indirection_el 会与 any_name_list 产生不可接受的 R/R 冲突。 */
indirection_el:
    '[' a_expr ']'
    {
        $$ = (Node *)makeA_Indices(false, NULL, $2);
    }
    | '[' opt_slice_bound ':' opt_slice_bound ']'
    {
        $$ = (Node *)makeA_Indices(true, $2, $4);
    }
    ;

opt_slice_bound:
    a_expr              { $$ = $1; }
    | /*EMPTY*/         { $$ = NULL; }
    ;

indirection:
    indirection_el              { $$ = list_make1($1); }
    | indirection indirection_el    { $$ = lappend($1, $2); }
    ;

any_name_list:
    name               { $$ = list_make1(makeStringConst($1, @1.first_line, 0)); }
    | any_name_list '.' name { $$ = lappend($1, makeStringConst($3, @3.first_line, 0)); }
    ;

any_name:
    name               { $$ = list_make1(makeString($1)); }
    | name attrs       { $$ = lcons(makeString($1), $2); }
    | STRING           { $$ = list_make1(makeString($1)); }
    ;

attrs:
    '.' name           { $$ = list_make1(makeString($2)); }
    | attrs '.' name   { $$ = lappend($1, makeString($3)); }

/* ReqID:TECC4.0 - 扩展name规则，允许关键字作标识符(TEMP/ZONE_P) */
name:
    IDENT              { $$ = $1; }
    | DEK              { $$ = "dek"; }
    | KEK              { $$ = "kek"; }
    | KEY              { $$ = "key"; }
    | FORMAT           { $$ = "format"; }
    | TIME_P           { $$ = "time"; }
    | TYPE             { $$ = "type"; }
    | TEXT             { $$ = "text"; }
    | VALUE_P          { $$ = "value"; }
    | TEMP             { $$ = "temp"; }
    | ZONE_P           { $$ = "zone"; }
    /* ReqID:TECC4.0 - SET 语句关键字允许作标识符（避免破坏现有 SQL） */
    | LOCAL            { $$ = "local"; }
    | SESSION          { $$ = "session"; }
    | TRANSACTION      { $$ = "transaction"; }
    | CHARACTERISTICS  { $$ = "characteristics"; }
    | CATALOG_P        { $$ = "catalog"; }
    | NAMES            { $$ = "names"; }
    | ROLE             { $$ = "role"; }
    | AUTHORIZATION    { $$ = "authorization"; }
    | XML_P            { $$ = "xml"; }
    | OPTION           { $$ = "option"; }
    | DOCUMENT         { $$ = "document"; }
    | CONTENT          { $$ = "content"; }
    | SNAPSHOT         { $$ = "snapshot"; }
    | ISOLATION        { $$ = "isolation"; }
    | LEVEL            { $$ = "level"; }
    | READ             { $$ = "read"; }
    | UNCOMMITTED      { $$ = "uncommitted"; }
    | COMMITTED        { $$ = "committed"; }
    | REPEATABLE       { $$ = "repeatable"; }
    | SERIALIZABLE     { $$ = "serializable"; }
    | ONLY             { $$ = "only"; }
    | WRITE            { $$ = "write"; }
    | CURRENT_P        { $$ = "current"; }
    ;

col_id:
    name               { $$ = $1; }
    ;

iconst:
    INTEGER            { $$ = atoll($1); }
    ;

sconst:
    STRING             { $$ = $1; }
    ;

stmt_name:
    IDENT              { $$ = $1; }
    ;

target_list:
    expr               {
        ResTarget *res = makeResTarget(NULL, $1, @1.first_line);
        $$ = list_make1((Node *)res);
    }
    | expr AS col_id   {
        ResTarget *res = makeResTarget($3, $1, @2.first_line);
        $$ = list_make1((Node *)res);
    }
    /* ReqID:TECC4.0 - Bare alias without AS keyword */
    | expr col_id      {
        ResTarget *res = makeResTarget($2, $1, @1.first_line);
        $$ = list_make1((Node *)res);
    }
    | target_list ',' expr {
        ResTarget *res = makeResTarget(NULL, $3, @3.first_line);
        $$ = lappend($1, (Node *)res);
    }
    | target_list ',' expr AS col_id {
        ResTarget *res = makeResTarget($5, $3, @4.first_line);
        $$ = lappend($1, (Node *)res);
    }
    /* ReqID:TECC4.0 - Bare alias continuation without AS keyword */
    | target_list ',' expr col_id {
        ResTarget *res = makeResTarget($4, $3, @3.first_line);
        $$ = lappend($1, (Node *)res);
    }
    | STAR             {
        /* SELECT * case - create a special representation */
        /* Instead of A_Star node, create a ResTarget with a ColumnRef containing "*" */
        {
            ColumnRef *cref = (ColumnRef *)tecc_palloc(sizeof(ColumnRef));
            ResTarget *res = (ResTarget *)tecc_palloc(sizeof(ResTarget));

            memset(cref, 0, sizeof(ColumnRef));
            cref->type = FE_T_ColumnRef;
            cref->fields = list_make1(makeStringConst("*", @1.first_line, 0));
            cref->location = @1.first_line;

            memset(res, 0, sizeof(ResTarget));
            res->type = FE_T_ResTarget;
            res->name = NULL;  /* No alias */
            res->indirection = NIL;
            res->val = (Node *)cref;
            res->location = @1.first_line;

            $$ = list_make1((Node *)res);
        }
    }
    | target_list ',' STAR {
        /* SELECT 'literal', * FROM ... */
        {
            ColumnRef *cref = (ColumnRef *)tecc_palloc(sizeof(ColumnRef));
            ResTarget *res = (ResTarget *)tecc_palloc(sizeof(ResTarget));

            memset(cref, 0, sizeof(ColumnRef));
            cref->type = FE_T_ColumnRef;
            cref->fields = list_make1(makeStringConst("*", @3.first_line, 0));
            cref->location = @3.first_line;

            memset(res, 0, sizeof(ResTarget));
            res->type = FE_T_ResTarget;
            res->name = NULL;
            res->indirection = NIL;
            res->val = (Node *)cref;
            res->location = @3.first_line;

            $$ = lappend($1, (Node *)res);
        }
    }
    /* ReqID:TECC4.0 - 空目标列表：PostgreSQL 允许 "SELECT FROM t WHERE ..."
     * （与后端 gram.y 的 target_list 空产生式一致）。缺少该产生式会
     * 导致此类语句前端解析失败被整体放行，密态常量明文发往服务端
     * （服务端报 invalid hex string）。 */
    | /* EMPTY */      { $$ = NIL; }
    ;

id_list:
    col_id             { $$ = list_make1($1); }
    | id_list ',' col_id { $$ = lappend($1, $3); }
    ;


expr_list:
    expr               { $$ = list_make1($1); }
    | expr_list ',' expr { $$ = lappend($1, $3); }
    ;

type_list:
    type_name              { $$ = list_make1($1); }
    | type_list ',' type_name { $$ = lappend($1, $3); }
    ;

/* Helper rules for SELECT statement */
opt_all_clause:
    /* empty */        {
        $$ = NIL;
    }
    | DISTINCT         {
        $$ = list_make1(makeIntConst(1, @1.first_line, 0));
    }
    | ALL              {
        $$ = NIL;
    }
    /* ReqID:TECC4.0 - DISTINCT ON (expr_list) */
    | DISTINCT ON '(' expr_list ')' {
        $$ = $4;
    }
    ;

opt_from_clause:
    /* empty */        { $$ = NIL; }
    | FROM from_list   { $$ = $2; }
    ;

from_list:
    table_ref          { $$ = list_make1($1); }
    | from_list ',' table_ref { $$ = lappend($1, $3); }
    ;

table_ref:
    basic_table_ref                              { $$ = $1; }
    | table_ref JOIN basic_table_ref ON expr
    {
        $$ = (Node *)makeJoinExpr(JOIN_INNER, $1, $3, $5, @2.first_line);
    }
    | table_ref INNER JOIN basic_table_ref ON expr
    {
        $$ = (Node *)makeJoinExpr(JOIN_INNER, $1, $4, $6, @2.first_line);
    }
    | table_ref LEFT opt_outer JOIN basic_table_ref ON expr
    {
        $$ = (Node *)makeJoinExpr(JOIN_LEFT, $1, $5, $7, @2.first_line);
    }
    | table_ref RIGHT opt_outer JOIN basic_table_ref ON expr
    {
        $$ = (Node *)makeJoinExpr(JOIN_RIGHT, $1, $5, $7, @2.first_line);
    }
    | table_ref FULL opt_outer JOIN basic_table_ref ON expr
    {
        $$ = (Node *)makeJoinExpr(JOIN_FULL, $1, $5, $7, @2.first_line);
    }
    | table_ref CROSS JOIN basic_table_ref
    {
        $$ = (Node *)makeJoinExpr(JOIN_CROSS, $1, $4, NULL, @2.first_line);
    }
    /* NATURAL JOIN — 隐式匹配同名列 */
    | table_ref NATURAL JOIN basic_table_ref
    {
        JoinExpr *j = makeJoinExpr(JOIN_INNER, $1, $4, NULL, @2.first_line);
        j->isNatural = true;
        $$ = (Node *)j;
    }
    | table_ref NATURAL LEFT JOIN basic_table_ref
    {
        JoinExpr *j = makeJoinExpr(JOIN_LEFT, $1, $5, NULL, @3.first_line);
        j->isNatural = true;
        $$ = (Node *)j;
    }
    | table_ref NATURAL RIGHT JOIN basic_table_ref
    {
        JoinExpr *j = makeJoinExpr(JOIN_RIGHT, $1, $5, NULL, @3.first_line);
        j->isNatural = true;
        $$ = (Node *)j;
    }
    | table_ref NATURAL FULL JOIN basic_table_ref
    {
        JoinExpr *j = makeJoinExpr(JOIN_FULL, $1, $5, NULL, @3.first_line);
        j->isNatural = true;
        $$ = (Node *)j;
    }
    /* JOIN ... USING — 用同名列连接 */
    | table_ref JOIN basic_table_ref USING '(' column_list ')'
    {
        JoinExpr *j = makeJoinExpr(JOIN_INNER, $1, $3, NULL, @2.first_line);
        j->usingClause = $6;
        $$ = (Node *)j;
    }
    | table_ref INNER JOIN basic_table_ref USING '(' column_list ')'
    {
        JoinExpr *j = makeJoinExpr(JOIN_INNER, $1, $4, NULL, @2.first_line);
        j->usingClause = $7;
        $$ = (Node *)j;
    }
    | table_ref LEFT opt_outer JOIN basic_table_ref USING '(' column_list ')'
    {
        JoinExpr *j = makeJoinExpr(JOIN_LEFT, $1, $5, NULL, @2.first_line);
        j->usingClause = $8;
        $$ = (Node *)j;
    }
    | table_ref RIGHT opt_outer JOIN basic_table_ref USING '(' column_list ')'
    {
        JoinExpr *j = makeJoinExpr(JOIN_RIGHT, $1, $5, NULL, @2.first_line);
        j->usingClause = $8;
        $$ = (Node *)j;
    }
    | table_ref FULL opt_outer JOIN basic_table_ref USING '(' column_list ')'
    {
        JoinExpr *j = makeJoinExpr(JOIN_FULL, $1, $5, NULL, @2.first_line);
        j->usingClause = $8;
        $$ = (Node *)j;
    }
    ;

basic_table_ref:
    qualified_name opt_table_alias
    {
        RangeVar *rv = (RangeVar *)$1;
        rv->alias = (Alias *)$2;
        $$ = (Node *)rv;
    }
    | '(' select_stmt ')' opt_table_alias
    {
        $$ = (Node *)makeRangeSubselect($2, (Alias *)$4, @1.first_line);
    }
    | '(' table_ref ')' opt_table_alias
    {
        $$ = $2;
    }
    | LATERAL '(' select_stmt ')' opt_table_alias
    {
        RangeSubselect *rss = makeRangeSubselect($3, (Alias *)$5, @1.first_line);
        rss->lateral = true;
        $$ = (Node *)rss;
    }
    | func_call opt_table_alias
    {
        $$ = (Node *)makeRangeFunction($1, (Alias *)$2, false, @1.first_line);
    }
    | LATERAL func_call opt_table_alias
    {
        $$ = (Node *)makeRangeFunction($2, (Alias *)$3, true, @2.first_line);
    }
    ;

opt_outer:
    /* empty */
    | OUTER
    ;

opt_table_alias:
    /* empty */                  { $$ = NULL; }
    | AS name                    { $$ = (Node *)makeAlias($2, NIL); }
    | AS name '(' id_list ')'   { $$ = (Node *)makeAlias($2, $4); }
    | name                       { $$ = (Node *)makeAlias($1, NIL); }
    | name '(' id_list ')'      { $$ = (Node *)makeAlias($1, $3); }
    ;

qualified_name:
    IDENT              { $$ = (Node *)makeRangeVar(NULL, $1, @1.first_line); }
    | IDENT '.' IDENT  { $$ = (Node *)makeRangeVar($1, $3, @1.first_line); }
    ;

/* ReqID:TECC4.0 - Target table with optional alias for UPDATE/DELETE */
relation_expr_opt_alias:
    qualified_name
    {
        $$ = $1;
    }
    | qualified_name name
    {
        RangeVar *rv = (RangeVar *)$1;
        rv->alias = makeAlias($2, NIL);
        $$ = (Node *)rv;
    }
    | qualified_name AS name
    {
        RangeVar *rv = (RangeVar *)$1;
        rv->alias = makeAlias($3, NIL);
        $$ = (Node *)rv;
    }
    ;

opt_where:
    /* empty */        { $$ = NULL; }
    | WHERE expr       { $$ = $2; }
    ;

opt_group:
    /* empty */        { $$ = NIL; }
    | GROUP BY expr_list { $$ = $3; }
    ;

opt_having:
    /* empty */        { $$ = NULL; }
    | HAVING expr      { $$ = $2; }
    ;

opt_window:
    /* empty */                         { $$ = NIL; }
    | WINDOW window_definition_list     { $$ = $2; }
    ;

opt_order:
    /* empty */        { $$ = NIL; }
    | ORDER BY sortby_list { $$ = $3; }
    ;

sortby_list:
    sortby                           { $$ = list_make1($1); }
    | sortby_list ',' sortby         { $$ = lappend($1, $3); }
    ;

sortby:
    b_expr opt_asc_desc opt_nulls_order
        {
            SortBy *sb = (SortBy *)tecc_palloc(sizeof(SortBy));
            sb->type = FE_T_SortBy;
            sb->node = $1;
            sb->sortby_dir = $2;
            sb->sortby_nulls = $3;
            sb->useOp = NIL;
            sb->location = @1.first_line;
            $$ = (Node *)sb;
        }
    /* ReqID:TECC4.0 - USING operator in ORDER BY */
    | b_expr USING any_oper opt_asc_desc opt_nulls_order
        {
            SortBy *sb = (SortBy *)tecc_palloc(sizeof(SortBy));
            sb->type = FE_T_SortBy;
            sb->node = $1;
            sb->sortby_dir = $4;
            sb->sortby_nulls = $5;
            sb->useOp = (List *)$3;
            sb->location = @1.first_line;
            $$ = (Node *)sb;
        }
    ;

opt_asc_desc:
    /* empty */        { $$ = SORTBY_DEFAULT; }
    | ASC              { $$ = SORTBY_ASC; }
    | DESC             { $$ = SORTBY_DESC; }
    ;

opt_nulls_order:
    /* empty */                { $$ = SORTBY_NULLS_DEFAULT; }
    | NULLS_P FIRST_P         { $$ = SORTBY_NULLS_FIRST; }
    | NULLS_P LAST_P          { $$ = SORTBY_NULLS_LAST; }
    ;

opt_limit:
    /* empty */        { $$ = NULL; }
    | LIMIT expr       { $$ = $2; }
    ;

opt_offset:
    /* empty */        { $$ = NULL; }
    | OFFSET expr      { $$ = $2; }
    ;

/* ReqID:TECC4.0 - Locking clause (FOR UPDATE/SHARE etc.) */
opt_locking_clause:
    /* empty */            { $$ = NIL; }
    | for_locking_items        { $$ = $1; }
    ;

for_locking_items:
    for_locking_item                   { $$ = list_make1($1); }
    | for_locking_items for_locking_item   { $$ = lappend($1, $2); }
    ;

for_locking_item:
    FOR for_locking_strength opt_locked_rels opt_nowait_or_skip
    {
        /* Store locking info as a DefElem: defname = lock strength str */
        DefElem *de = makeDefElem($2, (Node *)$3, @1.first_line);
        $$ = (Node *)de;
    }
    ;

for_locking_strength:
    UPDATE             { $$ = "for update"; }
    | NO KEY UPDATE        { $$ = "for no key update"; }
    | SHARE               { $$ = "for share"; }
    | KEY SHARE           { $$ = "for key share"; }
    ;

opt_locked_rels:
    /* empty */         { $$ = NIL; }
    | OF qualified_name_list  { $$ = $2; }
    ;

opt_nowait_or_skip:
    /* empty */  { $$ = NIL; }
    | NOWAIT     { $$ = list_make1(makeStringConst("nowait", @1.first_line, 0)); }
    | SKIP LOCKED { $$ = list_make1(makeStringConst("skip locked", @1.first_line, 0)); }
    ;

/* Helper rules for INSERT statement */
opt_column_list:
    /* empty */        { $$ = NIL; }
    | '(' id_list ')' { $$ = convert_string_list_to_restargets($2, @1.first_line); }
    ;

/* ReqID:TECC4.0 - INSERT OVERRIDING 选项 */
opt_overriding:
    /* empty */                { $$ = OVERRIDING_NOT_SET; }
    | OVERRIDING USER_P VALUE_P  { $$ = OVERRIDING_USER_VALUE; }
    | OVERRIDING SYSTEM_P VALUE_P { $$ = OVERRIDING_SYSTEM_VALUE; }
    ;

insert_rest:
    select_stmt      { $$ = $1; }
    | DEFAULT VALUES
    {
        SelectStmt *select = makeSelectStmt(@1.first_line, @$.last_column - @1.first_column + 1);
        select->defaultValues = true;
        $$ = (Node *)select;
    }
    ;

values_clause:
    VALUES '(' expr_list ')' {
        /* Create a simple SELECT statement for VALUES */
        SelectStmt *select = makeSelectStmt(@1.first_line, @$.last_column - @1.first_column + 1);

        /* For INSERT...VALUES, store the expression list in valuesLists */
        /* valuesLists is a list of lists: outer list = rows, inner list = column values */
        List *single_row_values = $3;  /* expr_list is already a List of expressions */
        List *values_lists = list_make1(single_row_values);

        select->valuesLists = values_lists;
        select->targetList = NIL;  /* targetList should be empty for VALUES clause */
        $$ = (Node *)select;
    }
    | values_clause ',' '(' expr_list ')' {
        /* Append more values to existing SELECT statement */
        SelectStmt *select = (SelectStmt *)$1;

        /* Add another row to valuesLists */
        if (select->valuesLists == NIL) {
            select->valuesLists = list_make1($4);
        } else {
            select->valuesLists = lappend(select->valuesLists, $4);
        }

        $$ = (Node *)select;
    }
    ;

opt_on_conflict:
    /* empty */        { $$ = NIL; }
    | ON CONFLICT conflict_target conflict_action
    {
        OnConflictClause *occ = (OnConflictClause *)$4;
        occ->infer = (Node *)$3;
        $$ = list_make1((Node *)occ);
    }
    ;

conflict_target:
    /* empty */          { $$ = NULL; }
    | '(' expr_list ')' opt_where
    {
        $$ = (Node *)makeInferClause((List *)$2, $4, NULL, @1.first_line);
    }
    | ON CONSTRAINT name
    {
        $$ = (Node *)makeInferClause(NIL, NULL, $3, @3.first_line);
    }
    ;

conflict_action:
    DO NOTHING
    {
        $$ = (Node *)makeOnConflictClause(ONCONFLICT_NOTHING, @1.first_line);
    }
    | DO UPDATE SET set_clause_list opt_where
    {
        OnConflictClause *occ = makeOnConflictClause(ONCONFLICT_UPDATE, @1.first_line);
        occ->targetList = $4;
        occ->whereClause = $5;
        $$ = (Node *)occ;
    }
    ;

/* Helper rules for UPDATE statement.
 * set_clause always returns a List*; concatenate for multiple SET clauses. */
set_clause_list:
    set_clause         { $$ = $1; }
    | set_clause_list ',' set_clause
    {
        ListCell *scl_lc;
        List *sc = (List *)$3;  /* cast from bison void* union */
        foreach(scl_lc, sc) { $1 = lappend($1, lfirst(scl_lc)); }
        $$ = $1;
    }
    ;

/* ReqID:TECC4.0 - SET clause: single-column and multi-column support.
 * Always returns a List* of ResTarget nodes. */
set_clause:
    col_id EQUAL expr    {
        ResTarget *target = makeResTarget($1, $3, @2.first_line);
        $$ = list_make1((Node *)target);
    }
    | '(' set_target_list ')' EQUAL expr
    {
        /* Multi-column SET: (col1, col2, ...) = expr.
         * Create a ResTarget for each target column with the shared expr.
         * Processor handles positional extraction from RowExpr/SubLink. */
        ListCell *lc;
        List *result = NIL;
        List *cols = (List *)$2;  /* cast from bison void* union */
        foreach(lc, cols)
        {
            char *colname = (char *) lfirst(lc);
            ResTarget *target = makeResTarget(colname, $5, @3.first_line);
            result = lappend(result, (Node *)target);
        }
        $$ = result;
    }
    ;

/* ReqID:TECC4.0 - Target column list for multi-column SET */
set_target_list:
    col_id    { $$ = list_make1($1); }
    | set_target_list ',' col_id    { $$ = lappend($1, $3); }
    ;

/* RETURNING clause — 用于 INSERT/UPDATE/DELETE */
opt_returning:
    /* empty */            { $$ = NIL; }
    | RETURNING target_list { $$ = $2; }
    ;

/* Helper rules for DELETE statement */
opt_using:
    /* empty */        { $$ = NIL; }
    | USING from_list  { $$ = $2; }
    ;

/* DROP behavior */
opt_drop_behavior:
    /* empty */        { $$ = DROP_RESTRICT; }
    | CASCADE          { $$ = DROP_CASCADE; }
    | RESTRICT         { $$ = DROP_RESTRICT; }
    ;

/* KEK and DEK options */
kek_opt_list:
    kek_param                     { $$ = list_make1($1); }
    | kek_opt_list ',' kek_param  { $$ = lappend($1, $3); }
    ;

dek_opt_list:
    dek_param                     { $$ = list_make1($1); }
    | dek_opt_list ',' dek_param  { $$ = lappend($1, $3); }
    ;

kek_param:
    IDENT EQUAL sconst
    {
        DefElem *elem = makeDefElem($1, (Node *)makeStringConst($3, @2.first_line, 0), @1.first_line);
        $$ = (Node *)elem;
    }
    | IDENT EQUAL IDENT
    {
        DefElem *elem = makeDefElem($1, (Node *)makeStringConst($3, @2.first_line, 0), @1.first_line);
        $$ = (Node *)elem;
    }
    | KEY_ALGO EQUAL sconst
    {
        DefElem *elem = makeDefElem("key_algo", (Node *)makeStringConst($3, @2.first_line, 0), @1.first_line);
        $$ = (Node *)elem;
    }
    | KEY_ALGO EQUAL IDENT
    {
        DefElem *elem = makeDefElem("key_algo", (Node *)makeStringConst($3, @2.first_line, 0), @1.first_line);
        $$ = (Node *)elem;
    }
    ;

dek_param:
    IDENT EQUAL sconst
    {
        /* ReqID:TECC4.0 - ENCRYPTED_VALUE is auto-generated, reject at parse time */
        DefElem *elem;
        if (strcasecmp($1, "encrypted_value") == 0)
        {
            tecc_yyerror("ENCRYPTED_VALUE is auto-generated and cannot be specified in CREATE DEK");
            YYABORT;
        }
        elem = makeDefElem($1, (Node *)makeStringConst($3, @2.first_line, 0), @1.first_line);
        $$ = (Node *)elem;
    }
    | IDENT EQUAL IDENT
    {
        /* ReqID:TECC4.0 - ENCRYPTED_VALUE is auto-generated, reject at parse time */
        DefElem *elem;
        if (strcasecmp($1, "encrypted_value") == 0)
        {
            tecc_yyerror("ENCRYPTED_VALUE is auto-generated and cannot be specified in CREATE DEK");
            YYABORT;
        }
        elem = makeDefElem($1, (Node *)makeStringConst($3, @2.first_line, 0), @1.first_line);
        $$ = (Node *)elem;
    }
    | KEK EQUAL any_name
    {
        char buf[NAMEDATALEN * 4];
        ListCell *lc;
        char *p = buf;
        foreach(lc, (List *)$3)
        {
            if (p != buf)
                *p++ = '.';
            p += sprintf(p, "%s", strVal((const Value *)lfirst(lc)));
        }
        $$ = (Node *)makeDefElem("kek", (Node *)makeStringConst(tecc_pstrdup(buf), @2.first_line, 0), @1.first_line);
    }
    | KEY_ALGO EQUAL sconst
    {
        DefElem *elem = makeDefElem("key_algo", (Node *)makeStringConst($3, @2.first_line, 0), @1.first_line);
        $$ = (Node *)elem;
    }
    | KEY_ALGO EQUAL IDENT
    {
        DefElem *elem = makeDefElem("key_algo", (Node *)makeStringConst($3, @2.first_line, 0), @1.first_line);
        $$ = (Node *)elem;
    }
    ;

/* Prepared statement and cursor statements */

PreparableStmt:
    select_stmt          { $$ = $1; }
    | insert_stmt        { $$ = $1; }
    | update_stmt        { $$ = $1; }
    | delete_stmt        { $$ = $1; }
    ;

prep_type_clause:
    '(' type_list ')'    { $$ = $2; }
    | /* EMPTY */        { $$ = NIL; }
    ;

execute_param_clause:
    '(' expr_list ')'    { $$ = $2; }
    | /* EMPTY */        { $$ = NIL; }
    ;

prepare_stmt:
    PREPARE stmt_name prep_type_clause AS PreparableStmt
    {
        $$ = (Node *)makePrepareStmt($2, $3, $5, @1.first_line);
    }
    ;

execute_stmt:
    EXECUTE stmt_name execute_param_clause
    {
        $$ = (Node *)makeExecuteStmt($2, $3, 0, NULL, NULL, NULL, NULL, @1.first_line);
    }
    ;

deallocate_stmt:
    DEALLOCATE name
    {
        $$ = (Node *)makeDeallocateStmt($2, @1.first_line);
    }
    | DEALLOCATE PREPARE name
    {
        $$ = (Node *)makeDeallocateStmt($3, @1.first_line);
    }
    | DEALLOCATE ALL
    {
        $$ = (Node *)makeDeallocateStmt(NULL, @1.first_line);
    }
    | DEALLOCATE PREPARE ALL
    {
        $$ = (Node *)makeDeallocateStmt(NULL, @1.first_line);
    }
    ;

opt_with_data:
    WITH DATA           { $$ = true; }
    | WITH NO DATA      { $$ = false; }
    | /* EMPTY */       { $$ = true; }
    ;

create_as_stmt:
    CREATE opt_temp TABLE qualified_name AS EXECUTE name execute_param_clause opt_with_data
    {
        ExecuteStmt *execStmt;
        CreateTableAsStmt *ctas;

        execStmt = makeExecuteStmt($7, $8, 0, NULL, NULL, NULL, NULL, @1.first_line);
        ctas = makeCreateTableAsStmt((RangeVar *)$4, (Node *)execStmt, false, @1.first_line);
        $$ = (Node *)ctas;
    }
    | CREATE opt_temp TABLE qualified_name AS select_stmt opt_with_data
    {
        CreateTableAsStmt *ctas;

        ctas = makeCreateTableAsStmt((RangeVar *)$4, (Node *)$6, false, @1.first_line);
        $$ = (Node *)ctas;
    }
    | CREATE opt_temp TABLE IF_P NOT EXISTS qualified_name AS EXECUTE name execute_param_clause opt_with_data
    {
        ExecuteStmt *execStmt;
        CreateTableAsStmt *ctas;

        execStmt = makeExecuteStmt($10, $11, 0, NULL, NULL, NULL, NULL, @1.first_line);
        ctas = makeCreateTableAsStmt((RangeVar *)$7, (Node *)execStmt, true, @1.first_line);
        $$ = (Node *)ctas;
    }
    | CREATE opt_temp TABLE IF_P NOT EXISTS qualified_name AS select_stmt opt_with_data
    {
        CreateTableAsStmt *ctas;

        ctas = makeCreateTableAsStmt((RangeVar *)$7, (Node *)$9, true, @1.first_line);
        $$ = (Node *)ctas;
    }
    ;

/* ReqID:TECC4.0 - 密态视图支持：CREATE [OR REPLACE] VIEW name [(cols)] AS select_stmt
 * 注意：TEMP/TEMPORARY VIEW 必须拆为独立规则，不可复用 opt_temp——
 * opt_temp 与 create_table_stmt 的 opt_temp 存在 shift/reduce 冲突。
 * 同时也不可复用 opt_or_replace——空的 opt_or_replace 仍会在 TEMP 前
 * 产生歧义（shift TEMP vs reduce empty opt_or_replace）。 */
view_stmt:
    CREATE VIEW qualified_name opt_column_list AS select_stmt
    {
        ViewStmt *n = makeViewStmt((RangeVar *)$3, (Node *)$6, false, false, $4, @1.first_line);
        $$ = (Node *)n;
    }
    | CREATE OR REPLACE VIEW qualified_name opt_column_list AS select_stmt
    {
        ViewStmt *n = makeViewStmt((RangeVar *)$5, (Node *)$8, true, false, $6, @1.first_line);
        $$ = (Node *)n;
    }
    | CREATE TEMP VIEW qualified_name opt_column_list AS select_stmt
    {
        ViewStmt *n = makeViewStmt((RangeVar *)$4, (Node *)$7, false, true, $5, @1.first_line);
        $$ = (Node *)n;
    }
    | CREATE OR REPLACE TEMP VIEW qualified_name opt_column_list AS select_stmt
    {
        ViewStmt *n = makeViewStmt((RangeVar *)$6, (Node *)$9, true, true, $7, @1.first_line);
        $$ = (Node *)n;
    }
    | CREATE TEMPORARY VIEW qualified_name opt_column_list AS select_stmt
    {
        ViewStmt *n = makeViewStmt((RangeVar *)$4, (Node *)$7, false, true, $5, @1.first_line);
        $$ = (Node *)n;
    }
    | CREATE OR REPLACE TEMPORARY VIEW qualified_name opt_column_list AS select_stmt
    {
        ViewStmt *n = makeViewStmt((RangeVar *)$6, (Node *)$9, true, true, $7, @1.first_line);
        $$ = (Node *)n;
    }
    ;

/* ReqID:TECC4.0 - 密态视图支持：CREATE MATERIALIZED VIEW name [(cols)] AS select_stmt [WITH [NO] DATA]
 * 复用 CreateTableAsStmt：objtype=1 表示物化视图，skipData 表示 WITH NO DATA */
matview_stmt:
    CREATE MATERIALIZED VIEW qualified_name opt_column_list AS select_stmt opt_with_data
    {
        CreateTableAsStmt *ctas;

        ctas = makeCreateTableAsStmt((RangeVar *)$4, (Node *)$7, false, @1.first_line);
        ctas->objtype = 1;      /* OBJECT_MATVIEW */
        ctas->skipData = !$8;   /* opt_with_data: true=WITH DATA, false=WITH NO DATA */
        $$ = (Node *)ctas;
    }
    ;

declare_stmt:
    DECLARE stmt_name CURSOR FOR select_stmt
    {
        $$ = (Node *)makeDeclareCursorStmt($2, $5, @1.first_line);
    }
    ;

copy_stmt:
    COPY qualified_name opt_column_list FROM STDIN copy_options
    {
        /* Full implementation for COPY FROM STDIN */
        $$ = (Node *)makeCopyStmt((struct RangeVar *)$2, NULL, $3, true, NULL, $6,
                                  @1.first_line, @$.last_line - @1.first_line + 1);
    }
    | COPY qualified_name opt_column_list TO STDOUT copy_options
    {
        /* Full implementation for COPY TO STDOUT */
        $$ = (Node *)makeCopyStmt((struct RangeVar *)$2, NULL, $3, false, NULL, $6,
                                  @1.first_line, @$.last_line - @1.first_line + 1);
    }
    | COPY '(' select_stmt ')' TO STDOUT copy_options
    {
        /* Implementation for COPY (SELECT ...) TO STDOUT */
        $$ = (Node *)makeCopyStmt(NULL, $3, NULL, false, NULL, $7,
                                  @1.first_line, @$.last_line - @1.first_line + 1);
    }
    ;

copy_options:
    /* empty */        { $$ = NIL; }
    | WITH '(' copy_option_list ')' { $$ = $3; }
    | '(' copy_option_list ')' { $$ = $2; }
    | WITH copy_option_list_nonempty { $$ = $2; } /* 支持 WITH CSV HEADER */
    | copy_option_list_nonempty { $$ = $1; } /* 支持裸选项: CSV HEADER */
    ;

copy_option_list_nonempty:
    copy_option             { $$ = list_make1($1); }
    | copy_option_list_nonempty copy_option { $$ = lappend($1, $2); }
    ;

copy_option_list:
    copy_option                     { $$ = list_make1($1); }
    | copy_option_list ',' copy_option { $$ = lappend($1, $3); }
    ;

copy_option:
    IDENT EQUAL sconst
    {
        DefElem *elem = makeDefElem($1, (Node *)makeStringConst($3, @2.first_line, 0), @1.first_line);
        $$ = (Node *)elem;
    }
    | FORMAT sconst
    {
        DefElem *elem = makeDefElem("format", (Node *)makeStringConst($2, @1.first_line, 0), @1.first_line);
        $$ = (Node *)elem;
    }
    | FORMAT CSV
    {
        /* FORMAT CSV (无引号关键字形式) — \copy 元命令构建的 SQL 使用此语法 */
        DefElem *elem = makeDefElem("format", (Node *)makeStringConst("csv", @1.first_line, 0), @1.first_line);
        $$ = (Node *)elem;
    }
    | FORMAT BINARY
    {
        /* FORMAT BINARY (无引号关键字形式) */
        DefElem *elem = makeDefElem("format", (Node *)makeStringConst("binary", @1.first_line, 0), @1.first_line);
        $$ = (Node *)elem;
    }
    | FORMAT TEXT
    {
        /* FORMAT TEXT (无引号关键字形式) — 默认格式 */
        DefElem *elem = makeDefElem("format", (Node *)makeStringConst("text", @1.first_line, 0), @1.first_line);
        $$ = (Node *)elem;
    }
    | DELIMITER sconst
    {
        DefElem *elem = makeDefElem("delimiter", (Node *)makeStringConst($2, @1.first_line, 0), @1.first_line);
        $$ = (Node *)elem;
    }
    | NULL_P sconst
    {
        DefElem *elem = makeDefElem("null", (Node *)makeStringConst($2, @1.first_line, 0), @1.first_line);
        $$ = (Node *)elem;
    }
    | ENCODING sconst
    {
        DefElem *elem = makeDefElem("encoding", (Node *)makeStringConst($2, @1.first_line, 0), @1.first_line);
        $$ = (Node *)elem;
    }
    | CSV
    {
        DefElem *elem = makeDefElem("format", (Node *)makeStringConst("csv", @1.first_line, 0), @1.first_line);
        $$ = (Node *)elem;
    }
    | BINARY
    {
        DefElem *elem = makeDefElem("format", (Node *)makeStringConst("binary", @1.first_line, 0), @1.first_line);
        $$ = (Node *)elem;
    }
    | TEXT
    {
        DefElem *elem = makeDefElem("format", (Node *)makeStringConst("text", @1.first_line, 0), @1.first_line);
        $$ = (Node *)elem;
    }
    | HEADER
    {
        /* HEADER 裸关键字形式（不带参数，等效于 HEADER true） */
        DefElem *elem = makeDefElem("header", (Node *)makeStringConst("true", @1.first_line, 0), @1.first_line);
        $$ = (Node *)elem;
    }
    | HEADER TRUE_P
    {
        /* HEADER true（显式 boolean 参数） */
        DefElem *elem = makeDefElem("header", (Node *)makeStringConst("true", @1.first_line, 0), @1.first_line);
        $$ = (Node *)elem;
    }
    | HEADER FALSE_P
    {
        /* HEADER false */
        DefElem *elem = makeDefElem("header", (Node *)makeStringConst("false", @1.first_line, 0), @1.first_line);
        $$ = (Node *)elem;
    }
    | QUOTE sconst
    {
        DefElem *elem = makeDefElem("quote", (Node *)makeStringConst($2, @1.first_line, 0), @1.first_line);
        $$ = (Node *)elem;
    }
    | ESCAPE sconst
    {
        DefElem *elem = makeDefElem("escape", (Node *)makeStringConst($2, @1.first_line, 0), @1.first_line);
        $$ = (Node *)elem;
    }
    | FORCE_QUOTE '(' id_list ')'
    {
        DefElem *elem = makeDefElem("force_quote", (Node *)$3, @1.first_line);
        $$ = (Node *)elem;
    }
    | FORCE_NOT_NULL '(' id_list ')'
    {
        DefElem *elem = makeDefElem("force_not_null", (Node *)$3, @1.first_line);
        $$ = (Node *)elem;
    }
    | FORCE_QUOTE STAR
    {
        /* FORCE_QUOTE * - 将所有列强制加引号，存储 "*" 字符串标记 */
        List *star_list = list_make1((Node *)makeStringConst("*", @1.first_line, 0));
        DefElem *elem = makeDefElem("force_quote", (Node *)star_list, @1.first_line);
        $$ = (Node *)elem;
    }
    | FREEZE
    {
        /* FREEZE 选项，相当于 FREEZE true */
        DefElem *elem = makeDefElem("freeze", (Node *)makeStringConst("true", @1.first_line, 0), @1.first_line);
        $$ = (Node *)elem;
    }
    | FORCE_NULL '(' id_list ')'
    {
        DefElem *elem = makeDefElem("force_null", (Node *)$3, @1.first_line);
        $$ = (Node *)elem;
    }
    ;

/*
 * EXPLAIN 语句规则
 * 支持 4 种语法变体，前端只识别结构，options 不解析留给后端
 */
explain_stmt:
    EXPLAIN explainable_stmt
        {
            ExplainStmt *n = (ExplainStmt *)tecc_palloc(sizeof(ExplainStmt));
            n->type = FE_T_ExplainStmt;
            n->query = $2;
            n->options = NIL;
            $$ = (Node *)n;
        }
    | EXPLAIN ANALYZE explainable_stmt
        {
            ExplainStmt *n = (ExplainStmt *)tecc_palloc(sizeof(ExplainStmt));
            n->type = FE_T_ExplainStmt;
            n->query = $3;
            n->options = NIL;
            $$ = (Node *)n;
        }
    | EXPLAIN ANALYZE VERBOSE explainable_stmt
        {
            ExplainStmt *n = (ExplainStmt *)tecc_palloc(sizeof(ExplainStmt));
            n->type = FE_T_ExplainStmt;
            n->query = $4;
            n->options = NIL;
            $$ = (Node *)n;
        }
    | EXPLAIN VERBOSE explainable_stmt
        {
            ExplainStmt *n = (ExplainStmt *)tecc_palloc(sizeof(ExplainStmt));
            n->type = FE_T_ExplainStmt;
            n->query = $3;
            n->options = NIL;
            $$ = (Node *)n;
        }
    | EXPLAIN '(' explain_option_list ')' explainable_stmt
        {
            ExplainStmt *n = (ExplainStmt *)tecc_palloc(sizeof(ExplainStmt));
            n->type = FE_T_ExplainStmt;
            n->query = $5;
            n->options = NIL;
            $$ = (Node *)n;
        }
    ;

explainable_stmt:
    select_stmt    { $$ = $1; }
    | insert_stmt  { $$ = $1; }
    | update_stmt  { $$ = $1; }
    | delete_stmt  { $$ = $1; }
    | execute_stmt { $$ = $1; }
    ;

explain_option_list:
    explain_option_elem
    | explain_option_list ',' explain_option_elem
    ;

any_oper:
    EQUAL          { $$ = list_make1(makeString("=")); }
    | NOT_EQUAL    { $$ = list_make1(makeString("<>")); }
    | LESS         { $$ = list_make1(makeString("<")); }
    | GREATER      { $$ = list_make1(makeString(">")); }
    | LESS_EQUAL   { $$ = list_make1(makeString("<=")); }
    | GREATER_EQUAL { $$ = list_make1(makeString(">=")); }
    ;

explain_option_elem:
    IDENT
    | IDENT IDENT
    /* ReqID:TECC4.0 - ON/TRUE/FALSE are reserved tokens, but they are
     * valid boolean values in EXPLAIN options such as SUMMARY ON. */
    | IDENT ON
    | IDENT TRUE_P
    | IDENT FALSE_P
    | IDENT EQUAL IDENT
    | IDENT EQUAL ON
    | IDENT EQUAL TRUE_P
    | IDENT EQUAL FALSE_P
    | IDENT EQUAL STRING
    | IDENT EQUAL INTEGER
    | IDENT EQUAL FLOAT
    | ANALYZE
    | VERBOSE
    ;

/*
 * WITH CTE (Common Table Expression) 规则
 * 支持: WITH cte AS (...) SELECT ...
 *       WITH cte1 AS (...), cte2 AS (...) SELECT ...
 *       WITH RECURSIVE cte AS (...) SELECT ...
 */
with_clause:
    WITH cte_list
    {
        $$ = (Node *)$2;
    }
    | WITH RECURSIVE cte_list
    {
        /* ReqID:TECC4.0 - Set recursive flag on all CTEs */
        set_cte_list_recursive((List *)$3);
        $$ = (Node *)$3;
    }
    ;

cte_list:
    cte
    {
        $$ = list_make1($1);
    }
    | cte_list ',' cte
    {
        $$ = lappend($1, $3);
    }
    ;

cte:
    name opt_column_list AS '(' select_stmt ')'
    {
        List *alias_names;
        List *col_list;
        CommonTableExpr *cte_node;
        ListCell *lc;

        col_list = $2;
        alias_names = NIL;

        if (col_list != NIL)
        {
            for (lc = list_head(col_list); lc != NULL; lc = lnext(lc))
            {
                ResTarget *rt = (ResTarget *)lfirst(lc);
                if (rt && rt->name)
                    alias_names = lappend(alias_names, makeString(rt->name));
            }
        }

        cte_node = makeCommonTableExpr($1, alias_names, $5, @1.first_line);
        $$ = (Node *)cte_node;
    }
    /* ReqID:TECC4.0 - Data-modifying CTE: INSERT/UPDATE/DELETE */
    | name opt_column_list AS '(' insert_stmt ')'
    {
        List *alias_names;
        List *col_list;
        CommonTableExpr *cte_node;
        ListCell *lc;
        col_list = $2;
        alias_names = NIL;
        if (col_list != NIL)
        {
            for (lc = list_head(col_list); lc != NULL; lc = lnext(lc))
            {
                ResTarget *rt = (ResTarget *)lfirst(lc);
                if (rt && rt->name)
                    alias_names = lappend(alias_names, makeString(rt->name));
            }
        }
        cte_node = makeCommonTableExpr($1, alias_names, $5, @1.first_line);
        $$ = (Node *)cte_node;
    }
    | name opt_column_list AS '(' update_stmt ')'
    {
        List *alias_names;
        List *col_list;
        CommonTableExpr *cte_node;
        ListCell *lc;
        col_list = $2;
        alias_names = NIL;
        if (col_list != NIL)
        {
            for (lc = list_head(col_list); lc != NULL; lc = lnext(lc))
            {
                ResTarget *rt = (ResTarget *)lfirst(lc);
                if (rt && rt->name)
                    alias_names = lappend(alias_names, makeString(rt->name));
            }
        }
        cte_node = makeCommonTableExpr($1, alias_names, $5, @1.first_line);
        $$ = (Node *)cte_node;
    }
    | name opt_column_list AS '(' delete_stmt ')'
    {
        List *alias_names;
        List *col_list;
        CommonTableExpr *cte_node;
        ListCell *lc;
        col_list = $2;
        alias_names = NIL;
        if (col_list != NIL)
        {
            for (lc = list_head(col_list); lc != NULL; lc = lnext(lc))
            {
                ResTarget *rt = (ResTarget *)lfirst(lc);
                if (rt && rt->name)
                    alias_names = lappend(alias_names, makeString(rt->name));
            }
        }
        cte_node = makeCommonTableExpr($1, alias_names, $5, @1.first_line);
        $$ = (Node *)cte_node;
    }
    ;

%%

/* Helper function implementations */

/* Helper function implementations */
static Node *make_column_ref_from_list(List *fields, int location) {
    return (Node *)makeColumnRef(fields, location);
}


static pg_attribute_unused() Node *make_func_call_from_list(List *funcname, List *args, int location) {
    return (Node *)makeFuncCall(funcname, args, location);
}

static Node *make_a_expr(int kind, List *name, Node *lexpr, Node *rexpr, int location) {
    return (Node *)makeAExpr(kind, name, lexpr, rexpr, location);
}

static Node *make_a_expr_escape(int kind, List *name, Node *lexpr, Node *rexpr, Node *escape, int location) {
    return (Node *)makeAExprWithEscape(kind, name, lexpr, rexpr, escape, location);
}

static Node *make_bool_expr(int boolop, List *args, int location) {
    return (Node *)makeBoolExpr(boolop, args, location);
}

/* ReqID:TECC4.0 - Expand `expr op ANY/SOME/ALL(ARRAY[...])` into OR/AND of
 * element comparisons.  `use_or` non-zero => ANY/SOME (OR), zero => ALL (AND).
 * A single element degrades to a bare comparison. */
static Node *make_any_all_array_expr(void *op_name, void *lhs, List *elements, int use_or, int location) {
    List *operName = (List *) op_name;
    Node *lexpr = (Node *) lhs;
    const char *opstr = (operName != NIL) ?
        strVal((Value *) linitial(operName)) : NULL;
    List *args = NIL;
    ListCell *lc;

    for (lc = list_head(elements); lc != NULL; lc = lnext(lc))
    {
        Node *elem = (Node *) lfirst(lc);
        /* Fresh operator-name list per comparison (no aliasing between nodes). */
        List *elem_name = (opstr != NULL) ? list_make1(makeString(opstr)) : operName;
        Node *cmp = make_a_expr(AEXPR_OP, elem_name, lexpr, elem, location);
        args = lappend(args, cmp);
    }
    if (list_length(args) == 0)
        return NULL;
    if (list_length(args) == 1)
        return (Node *) lfirst(list_head(args));
    return make_bool_expr(use_or ? OR_EXPR : AND_EXPR, args, location);
}

static Node *make_null_test(Node *arg, int nulltesttype, int location) {
    return (Node *)makeNullTest(arg, nulltesttype, location);
}

/* Convert a list of column name strings to a list of ResTarget nodes */
static List *convert_string_list_to_restargets(List *string_list, int location) {
    List *result = NIL;
    ListCell *lc;

    if (!string_list) return NIL;

    foreach(lc, string_list) {
        char *col_name = (char *)lfirst(lc);
        ResTarget *target = makeResTarget(col_name, NULL, location);
        result = lappend(result, target);
    }

    return result;
}

/* Parser entry point */
static pg_attribute_unused() List *tecc_parse_string(const char *str) {
    /* This would be implemented with Bison/Flex integration */
    /* For now, return empty list */
    return NIL;
}

/* Bison parser entry point */
int tecc_parse(void);

/* Bison error reporting function - defined by Bison in tecc_grammar.c */
/* void tecc_yyerror(const char *msg) is generated by Bison */

/* Bison error reporting function */
void tecc_yyerror(const char *msg) {
    extern int tecc_lineno;
    TECC_LOG(TECC_LOG_DEBUG1, "Parser error at line %d: %s", tecc_lineno, msg);
}

/* Alias for bison's expected error function */
void tecc_error(const char *msg) {
    tecc_yyerror(msg);
}

/* Public parser interface */
static pg_attribute_unused() Node *tecc_parse_query(const char *query) {
    /* TODO: Implement proper parsing with Bison */
    /* For now, use the existing simple parser */
    return tecc_parser_parse_node(query);
}
