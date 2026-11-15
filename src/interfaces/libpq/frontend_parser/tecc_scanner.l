%{
/*
 * Simplified scanner for TECC parser
 * Based on PostgreSQL scanner but simplified for TECC
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "tecc_nodes.h"
#include "tecc_parser.h"
#include "tecc_grammar.h"

/* Thread-local storage macro for MSVC compatibility.
 * MSVC does not allow 'extern' + '__declspec(thread)' in C mode
 * (error C2143/C2153).  Fall back to a plain global for MSVC;
 * on Windows libpq runs in the client's main thread anyway. */
#ifndef TECC_THREAD_LOCAL
#if defined(_MSC_VER)
#define TECC_THREAD_LOCAL __declspec(thread)
#define TECC_THREAD_LOCAL_EXTERN
#elif defined(__GNUC__) || defined(__clang__)
#define TECC_THREAD_LOCAL __thread
#define TECC_THREAD_LOCAL_EXTERN TECC_THREAD_LOCAL
#else
#define TECC_THREAD_LOCAL
#define TECC_THREAD_LOCAL_EXTERN TECC_THREAD_LOCAL
#endif
#endif

/* Global variables for parser */
extern int tecc_lineno;
extern char *tecc_text;
extern FILE *tecc_yyin;
extern TECC_STYPE tecc_lval;
extern TECC_LTYPE tecc_lloc;
extern TECC_THREAD_LOCAL_EXTERN const char *tecc_buffer_start;

/* Token location in the scan buffer (byte offset and length).
 * tecc__scan_buffer makes yytext point into the buffer allocated by
 * setup_parser_input, so yytext - tecc_buffer_start gives the correct
 * byte offset from the start of the SQL text. */
#define SET_TOKEN_LOCATION() \
    do { \
        tecc_lloc.first_line = yytext - tecc_buffer_start; \
        tecc_lloc.last_column = yyleng; \
    } while (0)

/* Update token location for EVERY token, not just constants.
 * The grammar uses @1.first_line (byte offset from SQL start) for
 * position-based scanning. Without this, IDENT, keyword, and operator
 * tokens retain stale positions from the last constant token. */
#define YY_USER_ACTION (void)(tecc_lloc.first_line = yytext - tecc_buffer_start, tecc_lloc.last_column = yyleng);

/* Flex will use its own memory management */

/* Helper: unescape C-style escape sequences in-place.
 * Handles: \\ \n \t \r \b \f \' \" \xHH \ooo
 * Modifies str in-place and returns the new length. */
static size_t tecc_unescape_c_string(char *str, size_t len) {
    char *src = str, *dst = str;
    size_t i;
    for (i = 0; i < len; i++) {
        if (src[i] == '\\' && i + 1 < len) {
            i++;
            switch (src[i]) {
                case 'n':  *dst++ = '\n'; break;
                case 't':  *dst++ = '\t'; break;
                case 'r':  *dst++ = '\r'; break;
                case 'b':  *dst++ = '\b'; break;
                case 'f':  *dst++ = '\f'; break;
                case '0': case '1': case '2': case '3':
                case '4': case '5': case '6': case '7': {
                    int val = 0, j;
                    for (j = 0; j < 3 && i + j < len && src[i+j] >= '0' && src[i+j] <= '7'; j++)
                        val = val * 8 + (src[i+j] - '0');
                    *dst++ = (char)val;
                    i += j - 1;
                    break;
                }
                case 'x': {
                    if (i + 2 < len) {
                        char hex[3] = {src[i+1], src[i+2], 0};
                        *dst++ = (char)strtol(hex, NULL, 16);
                        i += 2;
                    }
                    break;
                }
                default: *dst++ = src[i]; break;  /* \\, \', \" etc. */
            }
        } else {
            *dst++ = src[i];
        }
    }
    return dst - str;
}

/* Begin - ReqID:TECC4.0 - Literal buffer for GUC-aware string scanning */
#define TECC_LITERAL_BUF_SIZE 65536
static char tecc_lit_buf[TECC_LITERAL_BUF_SIZE];
static int  tecc_lit_len = 0;
static int  tecc_str_start_off = 0;  /* byte offset of opening quote */

/* GUC getter from tecc_parser.c */
extern bool tecc_parser_get_standard_conforming_strings(void);
/* End - ReqID:TECC4.0 - Literal buffer */

%}

%option nounput
%option noinput
%option header-file="tecc_scanner.h"
%option prefix="tecc_"

/* Begin - ReqID:TECC4.0 - XQ/XE start conditions for GUC-aware string scanning.
 * XQ: standard_conforming_strings=on  (backslash is literal, ' ends string)
 * XE: standard_conforming_strings=off (\' is a quote escape, \\ is backslash escape) */
%x XC XQ XE

DIGIT    [0-9]
LETTER   [a-zA-Z_\200-\377]
IDENT    {LETTER}({LETTER}|{DIGIT}|[$])*
INTEGER  {DIGIT}+
FLOAT    {DIGIT}+\.{DIGIT}*
REAL     ({DIGIT}+[Ee][-+]?{DIGIT}+|{DIGIT}+\.{DIGIT}*[Ee][-+]?{DIGIT}+|{DIGIT}*\.{DIGIT}+[Ee][-+]?{DIGIT}+)
/* STRING pattern removed — replaced by XQ/XE start-condition rules below */
QUOTED_IDENT \"([^\"]|\"\")*\"
ECONST   [Ee]\'(\\.|[^\'\\])*\'
WS       [ \t\r\n]

%%

"--"[^\n]*  {
    /* ignore line comment */
}

"/*"        {
    BEGIN(XC);
}

<XC>[^*/]+  {
    /* ignore comment content */
}

<XC>"*/"    {
    BEGIN(INITIAL);
}

<XC>[*/]    {
    /* ignore single * or / inside comment */
}

<XC><<EOF>> {
    /* unterminated block comment, ignore and continue */
}

{WS}+    {
    /* ignore whitespace */
}

(?i:SELECT)   {
    return TECC_TOK_SELECT;
}
(?i:INSERT)   { return TECC_TOK_INSERT; }
(?i:UPDATE)   { return TECC_TOK_UPDATE; }
(?i:DELETE)   { return TECC_TOK_DELETE; }
(?i:CREATE)   { return TECC_TOK_CREATE; }
(?i:DROP)       { return TECC_TOK_DROP; }
(?i:KEK)      { return TECC_TOK_KEK; }
(?i:DEK)      { return TECC_TOK_DEK; }
(?i:TABLE)    { return TECC_TOK_TABLE; }
(?i:VIEW)     { return TECC_TOK_VIEW; }
(?i:MATERIALIZED) { return TECC_TOK_MATERIALIZED; }
(?i:REPLACE)  { return TECC_TOK_REPLACE; }
(?i:REFRESH)  { return TECC_TOK_REFRESH; }
(?i:INDEX)    { return TECC_TOK_INDEX; }
(?i:WITH)     { return TECC_TOK_WITH; }
(?i:RECURSIVE) { return TECC_TOK_RECURSIVE; }
(?i:ENCRYPTED) { return TECC_TOK_ENCRYPTED; }
(?i:WHERE)    { return TECC_TOK_WHERE; }
(?i:FROM)     { return TECC_TOK_FROM; }
(?i:SET)      { return TECC_TOK_SET; }
(?i:LOCAL)         { return TECC_TOK_LOCAL; }
(?i:SESSION)       { return TECC_TOK_SESSION; }
(?i:TRANSACTION)   { return TECC_TOK_TRANSACTION; }
(?i:CHARACTERISTICS) { return TECC_TOK_CHARACTERISTICS; }
(?i:CATALOG)       { return TECC_TOK_CATALOG_P; }
(?i:NAMES)         { return TECC_TOK_NAMES; }
(?i:ROLE)          { return TECC_TOK_ROLE; }
(?i:AUTHORIZATION) { return TECC_TOK_AUTHORIZATION; }
(?i:XML)           { return TECC_TOK_XML_P; }
(?i:OPTION)        { return TECC_TOK_OPTION; }
(?i:DOCUMENT)      { return TECC_TOK_DOCUMENT; }
(?i:CONTENT)       { return TECC_TOK_CONTENT; }
(?i:SNAPSHOT)      { return TECC_TOK_SNAPSHOT; }
(?i:ISOLATION)     { return TECC_TOK_ISOLATION; }
(?i:LEVEL)         { return TECC_TOK_LEVEL; }
(?i:READ)          { return TECC_TOK_READ; }
(?i:UNCOMMITTED)   { return TECC_TOK_UNCOMMITTED; }
(?i:COMMITTED)     { return TECC_TOK_COMMITTED; }
(?i:REPEATABLE)    { return TECC_TOK_REPEATABLE; }
(?i:SERIALIZABLE)  { return TECC_TOK_SERIALIZABLE; }
(?i:ONLY)          { return TECC_TOK_ONLY; }
(?i:WRITE)         { return TECC_TOK_WRITE; }
(?i:CURRENT)       { return TECC_TOK_CURRENT_P; }
(?i:INTO)     { return TECC_TOK_INTO; }
(?i:VALUES)   { return TECC_TOK_VALUES; }
(?i:AND)      { return TECC_TOK_AND; }
(?i:OR)       { return TECC_TOK_OR; }
(?i:NOT)      { return TECC_TOK_NOT; }
(?i:NULL)     { return TECC_TOK_NULL_P; }
(?i:IS)       { return TECC_TOK_IS; }
(?i:BETWEEN)  { return TECC_TOK_BETWEEN; }
(?i:LIKE)     { return TECC_TOK_LIKE; }
(?i:ILIKE)   { return TECC_TOK_ILIKE; }
(?i:ARRAY)   { return TECC_TOK_ARRAY; }
"CURRENT_DATE"|"current_date" { return TECC_TOK_CURRENT_DATE; }
"CURRENT_TIME"|"current_time" { return TECC_TOK_CURRENT_TIME; }
"CURRENT_TIMESTAMP"|"current_timestamp" { return TECC_TOK_CURRENT_TIMESTAMP; }
(?i:IN)       { return TECC_TOK_IN; }
(?i:EXISTS)   { return TECC_TOK_EXISTS; }
(?i:ALL)      { return TECC_TOK_ALL; }
(?i:ANY)      { return TECC_TOK_ANY; }
(?i:SOME)     { return TECC_TOK_SOME; }
(?i:SYMMETRIC) { return TECC_TOK_SYMMETRIC; }
(?i:SIMILAR) { return TECC_TOK_SIMILAR; }
(?i:VARIADIC) { return TECC_TOK_VARIADIC; }
(?i:WITHIN)   { return TECC_TOK_WITHIN; }
(?i:GROUP)    { return TECC_TOK_GROUP; }
(?i:BY)       { return TECC_TOK_BY; }
(?i:HAVING)   { return TECC_TOK_HAVING; }
(?i:ORDER)    { return TECC_TOK_ORDER; }
(?i:ASC)      { return TECC_TOK_ASC; }
(?i:DESC)     { return TECC_TOK_DESC; }
(?i:AS)       {
    return TECC_TOK_AS;
}
(?i:LIMIT)    { return TECC_TOK_LIMIT; }
(?i:OFFSET)   { return TECC_TOK_OFFSET; }
(?i:DISTINCT) { return TECC_TOK_DISTINCT; }
(?i:UNION)    { return TECC_TOK_UNION; }
(?i:INTERSECT) { return TECC_TOK_INTERSECT; }
(?i:EXCEPT)   { return TECC_TOK_EXCEPT; }
(?i:JOIN)     { return TECC_TOK_JOIN; }
(?i:INNER)    { return TECC_TOK_INNER; }
(?i:LEFT)     { return TECC_TOK_LEFT; }
(?i:RIGHT)    { return TECC_TOK_RIGHT; }
(?i:FULL)     { return TECC_TOK_FULL; }
(?i:OUTER)    { return TECC_TOK_OUTER; }
(?i:ON)       { return TECC_TOK_ON; }
(?i:CASCADE)  { return TECC_TOK_CASCADE; }
(?i:RESTRICT)  { return TECC_TOK_RESTRICT; }
(?i:IF)    { return TECC_TOK_IF_P; }
(?i:PREPARE)  { return TECC_TOK_PREPARE; }
(?i:EXECUTE)  { return TECC_TOK_EXECUTE; }
(?i:DEALLOCATE) { return TECC_TOK_DEALLOCATE; }
(?i:DECLARE)  { return TECC_TOK_DECLARE; }
(?i:CURSOR)   { return TECC_TOK_CURSOR; }
(?i:FOR)      { return TECC_TOK_FOR; }
(?i:SHARE)   { return TECC_TOK_SHARE; }
(?i:NOWAIT) { return TECC_TOK_NOWAIT; }
(?i:SKIP)     { return TECC_TOK_SKIP; }
(?i:LOCKED) { return TECC_TOK_LOCKED; }
(?i:FETCH)    { return TECC_TOK_FETCH; }
(?i:MOVE)     { return TECC_TOK_MOVE; }
(?i:CLOSE)    { return TECC_TOK_CLOSE; }
(?i:COPY)  { return TECC_TOK_COPY; }
(?i:TO)     { return TECC_TOK_TO; }
(?i:RENAME) { return TECC_TOK_RENAME; }
(?i:TEMPORARY) { return TECC_TOK_TEMPORARY; }
(?i:TEMP) { return TECC_TOK_TEMP; }
(?i:STDIN)   { return TECC_TOK_STDIN; }
(?i:STDOUT)  { return TECC_TOK_STDOUT; }
(?i:BINARY) { return TECC_TOK_BINARY; }
(?i:CSV)    { return TECC_TOK_CSV; }
(?i:TEXT)   { return TECC_TOK_TEXT; }
(?i:FORMAT)  { return TECC_TOK_FORMAT; }
(?i:FREEZE)  { return TECC_TOK_FREEZE; }
"FORCE_NULL"|"force_null" { return TECC_TOK_FORCE_NULL; }
(?i:DELIMITER) { return TECC_TOK_DELIMITER; }
(?i:ENCODING) { return TECC_TOK_ENCODING; }
(?i:HEADER)   { return TECC_TOK_HEADER; }
(?i:QUOTE)    { return TECC_TOK_QUOTE; }
(?i:ESCAPE)   { return TECC_TOK_ESCAPE; }
"FORCE_QUOTE"|"force_quote" { return TECC_TOK_FORCE_QUOTE; }
"FORCE_NOT_NULL"|"force_not_null" { return TECC_TOK_FORCE_NOT_NULL; }
(?i:DEFAULT)  { return TECC_TOK_DEFAULT; }

(?i:AT)         { return TECC_TOK_AT_P; }
(?i:TIME)     { return TECC_TOK_TIME_P; }
(?i:ZONE)     { return TECC_TOK_ZONE_P; }

(?i:PRIMARY) { return TECC_TOK_PRIMARY; }
(?i:KEY) { return TECC_TOK_KEY; }
(?i:REFERENCES) { return TECC_TOK_REFERENCES; }
(?i:FOREIGN) { return TECC_TOK_FOREIGN; }
(?i:UNIQUE) {
    return TECC_TOK_UNIQUE;
}
(?i:CHECK) {
    return TECC_TOK_CHECK;
}
(?i:ADD)        { return TECC_TOK_ADD; }
(?i:COLUMN)  { return TECC_TOK_COLUMN; }
(?i:CONSTRAINT) { return TECC_TOK_CONSTRAINT; }
(?i:DATA)      { return TECC_TOK_DATA; }
(?i:TYPE)      { return TECC_TOK_TYPE; }
(?i:COLLATE) { return TECC_TOK_COLLATE; }
(?i:ALTER)    { return TECC_TOK_ALTER; }

(?i:UNLOGGED) { return TECC_TOK_UNLOGGED; }
(?i:OF) { return TECC_TOK_OF; }
(?i:TABLESPACE) { return TECC_TOK_TABLESPACE; }
(?i:PARTITION) { return TECC_TOK_PARTITION; }
(?i:RANGE) { return TECC_TOK_RANGE; }
(?i:LIST) { return TECC_TOK_LIST; }
(?i:HASH) { return TECC_TOK_HASH; }
(?i:DEFERRABLE) { return TECC_TOK_DEFERRABLE; }
(?i:INITIALLY) { return TECC_TOK_INITIALLY; }
(?i:DEFERRED) { return TECC_TOK_DEFERRED; }
(?i:IMMEDIATE) { return TECC_TOK_IMMEDIATE; }
(?i:GENERATED) { return TECC_TOK_GENERATED; }
(?i:IDENTITY) { return TECC_TOK_IDENTITY; }
(?i:ALWAYS) { return TECC_TOK_ALWAYS; }
(?i:STORED) { return TECC_TOK_STORED; }
(?i:EXCLUDE) { return TECC_TOK_EXCLUDE; }
(?i:COMMIT) { return TECC_TOK_COMMIT; }
(?i:PRESERVE) { return TECC_TOK_PRESERVE; }
(?i:ROWS) { return TECC_TOK_ROWS; }
(?i:INCLUDING) { return TECC_TOK_INCLUDING; }
(?i:EXCLUDING) { return TECC_TOK_EXCLUDING; }
(?i:TRUE) { return TECC_TOK_TRUE_P; }
(?i:FALSE) { return TECC_TOK_FALSE_P; }
(?i:UNKNOWN) { return TECC_TOK_UNKNOWN; }
(?i:MATCH) { return TECC_TOK_MATCH; }
(?i:PARTIAL) { return TECC_TOK_PARTIAL; }
(?i:SIMPLE) { return TECC_TOK_SIMPLE; }
(?i:ACTION) { return TECC_TOK_ACTION; }
(?i:NO) { return TECC_TOK_NO; }
(?i:INHERITS) { return TECC_TOK_INHERITS; }
(?i:INHERIT) { return TECC_TOK_INHERIT; }
(?i:USING) { return TECC_TOK_USING; }
(?i:RETURNING) { return TECC_TOK_RETURNING; }
(?i:WINDOW) { return TECC_TOK_WINDOW; }
(?i:SEQUENCE) { return TECC_TOK_SEQUENCE; }
(?i:SCHEMA) { return TECC_TOK_SCHEMA; }
(?i:FUNCTION) { return TECC_TOK_FUNCTION; }

(?i:CROSS) { return TECC_TOK_CROSS; }
(?i:NATURAL) { return TECC_TOK_NATURAL; }
(?i:CASE) { return TECC_TOK_CASE; }
(?i:WHEN) { return TECC_TOK_WHEN; }
(?i:THEN) { return TECC_TOK_THEN; }
(?i:ELSE) { return TECC_TOK_ELSE_P; }
(?i:END) { return TECC_TOK_END_P; }
(?i:FILTER) { return TECC_TOK_FILTER; }
(?i:OVER) { return TECC_TOK_OVER; }
(?i:NULLS) { return TECC_TOK_NULLS_P; }
(?i:FIRST) { return TECC_TOK_FIRST_P; }
(?i:LAST) { return TECC_TOK_LAST_P; }
(?i:CAST) { return TECC_TOK_CAST; }
(?i:ROW) { return TECC_TOK_ROW; }
(?i:DO) { return TECC_TOK_DO; }
(?i:NOTHING) { return TECC_TOK_NOTHING; }
(?i:OPERATOR) { return TECC_TOK_OPERATOR; } /* ReqID:TECC4.0 */
(?i:CONFLICT) { return TECC_TOK_CONFLICT; }
(?i:LATERAL) { return TECC_TOK_LATERAL; }

(?i:OVERRIDING) { return TECC_TOK_OVERRIDING; }
(?i:USER) { return TECC_TOK_USER_P; }
(?i:SYSTEM) { return TECC_TOK_SYSTEM_P; }
(?i:VALUE) { return TECC_TOK_VALUE_P; }
(?i:MAXVALUE) { return TECC_TOK_MAXVALUE; }
(?i:MINVALUE) { return TECC_TOK_MINVALUE; }
(?i:ATTACH) { return TECC_TOK_ATTACH; }
(?i:DETACH) { return TECC_TOK_DETACH; }

(?i:EXPLAIN) { return TECC_TOK_EXPLAIN; }
(?i:ANALYZE) { return TECC_TOK_ANALYZE; }
(?i:VERBOSE) { return TECC_TOK_VERBOSE; }

"+"        { return TECC_TOK_PLUS; }
"-"        { return TECC_TOK_MINUS; }
"*"        { return TECC_TOK_STAR; }
"/"        { return TECC_TOK_SLASH; }
"%"        { return TECC_TOK_PERCENT; }
"="        { return TECC_TOK_EQUAL; }
"!="       { return TECC_TOK_NOT_EQUAL; }
"<>"       { return TECC_TOK_NOT_EQUAL; }
"!~"       { return TECC_TOK_REGEX_NOT_MATCH; }
"!~*"      { return TECC_TOK_REGEX_NOT_MATCH_CI; }
"~"        { return TECC_TOK_REGEX_MATCH; }
"~*"       { return TECC_TOK_REGEX_MATCH_CI; }
"<"        { return TECC_TOK_LESS; }
">"        { return TECC_TOK_GREATER; }
"<="       { return TECC_TOK_LESS_EQUAL; }
">="       { return TECC_TOK_GREATER_EQUAL; }
"||"       { return TECC_TOK_CONCAT; }
"::"       { return TECC_TOK_TYPECAST; }

";"        {
    return TECC_TOK_SEMICOLON;
}
","        { return ','; }
"."        { return '.'; }
"("        { return '('; }
")"        { return ')'; }
"["        { return '['; }
"]"        { return ']'; }

{QUOTED_IDENT} {
    char *str;
    size_t len;

    SET_TOKEN_LOCATION();
    /* Strip double quotes */
    str = yytext;
    len = strlen(str);
    if (len >= 2) {
        str[len-1] = '\0';
        tecc_lval.str = strdup(str+1);
    } else {
        tecc_lval.str = strdup("");
    }
    return TECC_TOK_IDENT;
}

{IDENT}    {
    tecc_lval.str = strdup(yytext);
    return TECC_TOK_IDENT;
}

{INTEGER}  {
    SET_TOKEN_LOCATION();
    tecc_lval.str = strdup(yytext);
    return TECC_TOK_INTEGER;
}

"$"{INTEGER} {
    /* $N parameter - extract the integer value */
    char *num_str = yytext + 1; /* Skip '$' */
    tecc_lval.ival = atoi(num_str);
    return TECC_TOK_PARAM;
}

{REAL}     {
    SET_TOKEN_LOCATION();
    tecc_lval.str = strdup(yytext);
    return TECC_TOK_FLOAT;
}

{FLOAT}    {
    SET_TOKEN_LOCATION();
    tecc_lval.str = strdup(yytext);
    return TECC_TOK_FLOAT;
}

"\'"    {
    /* Opening single quote — enter appropriate string scanning mode */
    tecc_str_start_off = yytext - tecc_buffer_start;
    tecc_lit_len = 0;
    if (tecc_parser_get_standard_conforming_strings())
        BEGIN(XQ);
    else
        BEGIN(XE);
}

<XQ>"\'\'"   {
    /* Doubled quote in standard conforming mode → literal '' */
    if (tecc_lit_len + 2 < TECC_LITERAL_BUF_SIZE) {
        tecc_lit_buf[tecc_lit_len++] = '\'';
        tecc_lit_buf[tecc_lit_len++] = '\'';
    }
}
<XQ>[^']+    {
    /* Non-quote characters */
    int n = yyleng;
    if (tecc_lit_len + n >= TECC_LITERAL_BUF_SIZE)
        n = TECC_LITERAL_BUF_SIZE - 1 - tecc_lit_len;
    if (n > 0) {
        memcpy(tecc_lit_buf + tecc_lit_len, yytext, n);
        tecc_lit_len += n;
    }
}
<XQ>"\'"    {
    /* Closing quote in standard conforming mode */
    BEGIN(INITIAL);
    tecc_lit_buf[tecc_lit_len] = '\0';
    tecc_lval.str = strdup(tecc_lit_buf);
    /* Restore token location to the opening quote; last_column = full match
     * length (content + 2 quotes), matching old STRING regex behavior */
    tecc_lloc.first_line = tecc_str_start_off;
    tecc_lloc.last_column = tecc_lit_len + 2;
    return TECC_TOK_STRING;
}

<XE>"\'\'"   {
    /* Doubled quote in escape-aware mode → literal '' */
    if (tecc_lit_len + 2 < TECC_LITERAL_BUF_SIZE) {
        tecc_lit_buf[tecc_lit_len++] = '\'';
        tecc_lit_buf[tecc_lit_len++] = '\'';
    }
}
<XE>"\\'"    {
    /* Escaped quote in escape-aware mode → literal ' */
    if (tecc_lit_len + 1 < TECC_LITERAL_BUF_SIZE)
        tecc_lit_buf[tecc_lit_len++] = '\'';
}
<XE>"\\\\"   {
    /* Escaped backslash in escape-aware mode → literal \ */
    if (tecc_lit_len + 1 < TECC_LITERAL_BUF_SIZE)
        tecc_lit_buf[tecc_lit_len++] = '\\';
}
<XE>"\\"[^'\\\n]  {
    /* Other backslash sequence → keep as-is (backslash + char) */
    if (tecc_lit_len + 2 < TECC_LITERAL_BUF_SIZE) {
        tecc_lit_buf[tecc_lit_len++] = '\\';
        tecc_lit_buf[tecc_lit_len++] = yytext[1];
    }
}
<XE>[^'\\]+  {
    /* Non-quote, non-backslash characters */
    int n = yyleng;
    if (tecc_lit_len + n >= TECC_LITERAL_BUF_SIZE)
        n = TECC_LITERAL_BUF_SIZE - 1 - tecc_lit_len;
    if (n > 0) {
        memcpy(tecc_lit_buf + tecc_lit_len, yytext, n);
        tecc_lit_len += n;
    }
}
<XE>"\'"    {
    /* Closing quote in escape-aware mode (unescaped ' ends the string) */
    BEGIN(INITIAL);
    tecc_lit_buf[tecc_lit_len] = '\0';
    tecc_lval.str = strdup(tecc_lit_buf);
    tecc_lloc.first_line = tecc_str_start_off;
    tecc_lloc.last_column = tecc_lit_len + 2;
    return TECC_TOK_STRING;
}
<XQ><<EOF>>  {
    /* Unterminated string in standard conforming mode */
    BEGIN(INITIAL);
    tecc_lit_buf[tecc_lit_len] = '\0';
    tecc_lval.str = strdup(tecc_lit_buf);
    tecc_lloc.first_line = tecc_str_start_off;
    tecc_lloc.last_column = tecc_lit_len + 2;
    return TECC_TOK_STRING;
}
<XE><<EOF>>  {
    /* Unterminated string in escape-aware mode */
    BEGIN(INITIAL);
    tecc_lit_buf[tecc_lit_len] = '\0';
    tecc_lval.str = strdup(tecc_lit_buf);
    tecc_lloc.first_line = tecc_str_start_off;
    tecc_lloc.last_column = tecc_lit_len + 2;
    return TECC_TOK_STRING;
}

{ECONST}   {
    char *str;
    size_t len;

    SET_TOKEN_LOCATION();
    /* Skip "E'" prefix, unescape content, strip closing "'" */
    str = yytext;
    len = strlen(str);
    if (len >= 3 && (str[0] == 'E' || str[0] == 'e') && str[1] == '\'') {
        /* Find closing quote */
        size_t inner_len;
        char *inner;
        str[len - 1] = '\0';
        inner = strdup(str + 2);
        inner_len = strlen(inner);
        inner_len = tecc_unescape_c_string(inner, inner_len);
        inner[inner_len] = '\0';
        tecc_lval.str = inner;
    } else {
        tecc_lval.str = strdup("");
    }
    return TECC_TOK_ECONST;
}

.          {
    /* Return character as token */
    return yytext[0];
}

%%

/* Additional C code */
int tecc_wrap(void) {
    return 1;
}

/* tecc_yyerror is defined in tecc_grammar.y */

