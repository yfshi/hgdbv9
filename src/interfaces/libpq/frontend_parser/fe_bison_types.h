/* Begin - ReqID:TECC4.0 */
/*
 * File: fe_bison_types.h
 * Abstract:
 *   Frontend version of Bison/Flex type definitions for TECC frontend parser.
 *   This file provides simplified type definitions to avoid dependency on
 *   backend bison/flex headers.
 *
 * Authored by <wangxp@highgo.com>, 20260306.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/frontend_parser/fe_bison_types.h
 */

#ifndef FE_BISON_TYPES_H
#define FE_BISON_TYPES_H

#include <setjmp.h>

/* ReqID:TECC4.0 - YYLTYPE definition */
typedef struct YYLTYPE
{
    int first_line;
    int first_column;
    int last_line;
    int last_column;
} YYLTYPE;

#define YYLTYPE_IS_DECLARED 1
#define YYLTYPE_IS_TRIVIAL 1

/* ReqID:TECC4.0 - core_yyscan_t definition (scanner context) */
typedef void* core_yyscan_t;

/* ReqID:TECC4.0 - PGClientLogic forward declaration */
typedef struct PGClientLogic PGClientLogic;

/* ReqID:TECC4.0 - Maximum lookahead length */
#define MAX_LOOKAHEAD_LEN 3

/* ReqID:TECC4.0 - Core scanner extra data type (simplified version) */
typedef struct core_yy_extra_type
{
    /* Simplified version - we'll add fields as needed */
    void *placeholder;
} core_yy_extra_type;

/* ReqID:TECC4.0 - Frontend core scanner extra data type */
typedef struct fe_core_yy_extra_type
{
    core_yy_extra_type base;
    jmp_buf jump_buffer;
    PGClientLogic *m_clientLogic;
    void *query_string_locationlist; /* List* type, defined as void* for now */
    int paren_depth;                 /* record the current depth in the '(' and ')' */
} fe_core_yy_extra_type;

/* ReqID:TECC4.0 - Include all necessary type definitions for %union */
#include "fe_adapter.h"

/* ReqID:TECC4.0 - Forward declarations for types used in %union */
/* These are already defined in fe_adapter.h */

/* ReqID:TECC4.0 - core_YYSTYPE definition (bison stack value type) */
typedef union core_YYSTYPE
{
    int                     ival;
    char                    *str;
    const char              *keyword;
    char                    chr;
    bool                    boolean;
    double                  dval;
    void                    *ptr;

    /* Types from fe_adapter.h for %union compatibility */
    DropBehavior            dbehavior;
    List                    *list;
    Node                    *node;
    Value                   *value;
    ObjectType              objtype;
    /* Note: TypeName and other complex types are defined as void* for now */
    void                    *typnam;
    void                    *fun_param;
    void                    *fun_param_mode;
    void                    *defelt;
    void                    *sortby;
    void                    *windef;
    void                    *keep;
    void                    *jexpr;
    void                    *ielem;
    void                    *alias;
    void                    *range;
    void                    *into;
    void                    *with;
    void                    *aind;
    void                    *target;
    void                    *privtarget;
    void                    *istmt;
    void                    *vsetstmt;
} core_YYSTYPE;

/* ReqID:TECC4.0 - Frontend scanner extra data type */
typedef struct fe_base_yy_extra_type
{
    /* Fields used by the core scanner */
    fe_core_yy_extra_type core_yy_extra;

    /* State variables for base_yylex() */
    int lookahead_num;                                /* lookahead num. Currently can be:0,1,2 */
    int lookahead_token[MAX_LOOKAHEAD_LEN];           /* token lookahead type */
    core_YYSTYPE lookahead_yylval[MAX_LOOKAHEAD_LEN]; /* yylval for lookahead token */
    YYLTYPE lookahead_yylloc[MAX_LOOKAHEAD_LEN];      /* yylloc for lookahead token */

    /* State variables that belong to the grammar */
    void *parsetree; /* List* type, final parse result is delivered here */
} fe_base_yy_extra_type;

/* ReqID:TECC4.0 - Scanner function declarations */
#ifdef __cplusplus
extern "C" {
#endif

fe_base_yy_extra_type *fe_pg_yyget_extra(core_yyscan_t yyscanner);
void fe_pg_yyset_extra(fe_base_yy_extra_type *extra, core_yyscan_t yyscanner);

#ifdef __cplusplus
}
#endif

/* ReqID:TECC4.0 - Bison/Flex compatibility macros */
#define YYLLOC_DEFAULT(Current, Rhs, N) \
    do { \
        if (N) \
            (Current) = (Rhs)[1]; \
        else \
            (Current) = (Rhs)[0]; \
    } while (0)

/* ReqID:TECC4.0 - Parser function declarations */
#ifdef __cplusplus
extern "C" {
#endif

/* Base parser error function */
void base_yyerror(YYLTYPE *yylloc, core_yyscan_t yyscanner, const char *msg);

/* Parser entry point */
int base_yyparse(core_yyscan_t yyscanner);

/* Parser initialization */
void fe_parser_init(fe_base_yy_extra_type *yyext);

#ifdef __cplusplus
}
#endif

#endif /* FE_BISON_TYPES_H */
/* End - ReqID:TECC4.0 */