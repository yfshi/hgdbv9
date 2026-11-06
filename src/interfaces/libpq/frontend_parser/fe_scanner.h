/* Begin - ReqID:TECC4.0 */
/*
 * File: fe_scanner.h
 * Abstract:
 *   Frontend scanner function declarations for TECC frontend parser.
 *   Simplified version to avoid dependency on backend scanner headers.
 *
 * Authored by <wangxp@highgo.com>, 20260306.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/frontend_parser/fe_scanner.h
 */

#ifndef FE_SCANNER_H
#define FE_SCANNER_H

#include "fe_bison_types_simple.h"

/* ReqID:TECC4.0 - Scan keyword list structure (simplified) */
typedef struct ScanKeywordList
{
    const char * const *keywords;    /* array of keyword strings */
    const uint16 *keyword_tokens;    /* array of token codes */
    int num_keywords;                /* number of keywords in array */
} ScanKeywordList;

/* ReqID:TECC4.0 - Memory utility functions (from feparser_memutils.h) */
void *feparser_malloc(size_t size);
void *feparser_malloc0(size_t size);
void *feparser_realloc(void *ptr, size_t size);
char *feparser_strdup(const char *str);
char *feparser_strndup(const char *str, size_t n);
void feparser_free(void *ptr);
void feparser_free_all(void);

/* ReqID:TECC4.0 - Scanner function declarations */
#ifdef __cplusplus
extern "C" {
#endif

/* Scanner initialization */
core_yyscan_t fe_scanner_init(const char *str, fe_core_yy_extra_type *yyext,
                              const ScanKeywordList *keywordlist, const uint16 *keyword_tokens);

/* Scanner cleanup */
void scanner_finish(core_yyscan_t yyscanner);

/* Core lexer function */
int core_yylex(core_YYSTYPE *lvalp, YYLTYPE *llocp, core_yyscan_t yyscanner);

/* Keyword list declarations (simplified) */
extern const ScanKeywordList ScanKeywords;
extern const uint16 ScanKeywordTokens[];

#ifdef __cplusplus
}
#endif

#endif /* FE_SCANNER_H */
/* End - ReqID:TECC4.0 */