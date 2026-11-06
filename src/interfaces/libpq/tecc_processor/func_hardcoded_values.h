/* Begin - ReqID:TECC4.0 */
/*
 * File: func_hardcoded_values.h
 *
 * Abstract:
 *   TECC function hardcoded values processor for handling hardcoded values in function bodies.
 *   Processes hardcoded values in CREATE FUNCTION, CREATE PROCEDURE, and DO statements.
 *
 * Authored by <wangxp@highgo.com>, 20260411.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/func_hardcoded_values.h
 */

#ifndef TECC_FUNC_HARDCODED_VALUES_H
#define TECC_FUNC_HARDCODED_VALUES_H

#include "tecc_processor_types.h"

/* Include PostgreSQL headers */
#include "postgres_fe.h"
#include "../libpq-fe.h"

/* Include frontend parser types */
#include "../frontend_parser/tecc_nodes.h"

/*
 * Function body structure
 */
typedef struct TeccFuncBody {
    char *partial_body;          /* Partial function body */
    size_t body_location;        /* Body location in original query */
    bool to_parse;               /* Whether to parse */
} TeccFuncBody;

/*
 * Delimiter type enumeration
 */
typedef enum TeccDelimiterType {
    TECC_DELIMITER_NONE = 0,     /* No delimiter */
    TECC_DELIMITER_HYPHEN,       /* Hyphen delimiter */
    TECC_DELIMITER_DOLLAR,       /* Dollar sign delimiter */
    TECC_DELIMITER_AS            /* AS keyword delimiter */
} TeccDelimiterType;

/*
 * Function parse information structure
 */
typedef struct TeccFuncParseInfo {
    bool found_begin;            /* Whether "begin" keyword found */
    bool found_if;               /* Whether "if " keyword found */
    bool found_end;              /* Whether "end " keyword found */
    bool while_loop;             /* Whether "while " keyword found */
    bool for_loop;               /* Whether "for " keyword found */
    bool skip_to_endline;        /* Whether to skip to end of line */
    bool in_for_loop;            /* Whether in FOR...LOOP syntax */
    bool is_split;               /* Whether to split as independent query */
    bool is_parse;               /* Whether to pass to SQL parser */
    bool in_string;              /* Whether in string */
} TeccFuncParseInfo;

/*
 * Function hardcoded values processor context structure
 */
typedef struct TeccFuncHardcodedValuesContext {
    /* Function body information */
    char *function_body;         /* Function body string */
    size_t body_size;            /* Function body size */
    size_t body_location;        /* Function body location */

    /* Delimiter information */
    size_t delimiter_location;   /* Delimiter location */
    size_t delimiter_size;       /* Delimiter size */
    TeccDelimiterType delimiter_type; /* Delimiter type */

    /* Language information */
    const char *language;        /* Function language */

    /* Status flags */
    bool is_do_stmt;             /* Whether DO statement */
    bool has_encrypted_columns;  /* Whether contains encrypted columns */
} TeccFuncHardcodedValuesContext;

/*
 * Public interface functions
 */

/* Process hardcoded values in function body */
bool tecc_func_hardcoded_values_process(
    const List *options,
    TeccStatementData *statement_data,
    bool is_do_stmt);

/* Split function body into multiple SQL statements */
TeccFuncBody **tecc_func_hardcoded_values_make_sql_body(
    const char *body,
    size_t body_size,
    size_t body_location,
    size_t *bodies_size);

/* Split function body */
TeccFuncBody **tecc_func_hardcoded_values_split_body(
    char *body,
    size_t body_size,
    size_t body_location,
    size_t *bodies_size);

/* Parse function body */
bool tecc_func_hardcoded_values_parse_body(
    const char *language,
    TeccFuncBody *body,
    size_t delimiter_location,
    size_t delimiter_size,
    size_t *body_location_offset,
    TeccStatementData *statement_data);

/* Adjust original statement */
bool tecc_func_hardcoded_values_adjust_original_stmt(
    const char *language,
    TeccStatementData *statement_data,
    const char *adjusted_body,
    size_t body_location,
    size_t delimiter_location,
    size_t delimiter_size,
    size_t partial_body_size);

/* Extract option information */
bool tecc_func_hardcoded_values_extract_def_elem_options(
    const List *options,
    bool is_do_stmt,
    DefElem **as_item,
    DefElem **language_item,
    size_t *delimiter_location,
    size_t *delimiter_size,
    const TeccStatementData *statement_data);

/* Process options */
bool tecc_func_hardcoded_values_process_options(
    const List *options,
    bool is_do_stmt,
    char **body,
    size_t *delimiter_location,
    size_t *delimiter_size,
    size_t *body_location,
    size_t *body_size,
    const char **language,
    const TeccStatementData *statement_data);

#endif /* TECC_FUNC_HARDCODED_VALUES_H */
/* End - ReqID:TECC4.0 */