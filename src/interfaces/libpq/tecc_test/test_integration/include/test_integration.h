/* Begin - ReqID:TECC4.0 */
/*
 * File: test_integration.h
 * Abstract:
 *   Integration test framework header for TECC processor and parser modules.
 *   Provides common definitions, structures, and function declarations.
 *
 * Authored by <wangxp@highgo.com>, 20260415.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_test/test_integration/include/test_integration.h
 */

#ifndef TEST_INTEGRATION_H
#define TEST_INTEGRATION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

/* Define FRONTEND for frontend compilation */
#define FRONTEND 1
#define FRONTEND_PARSER 1
#define TECC_TEST_INTEGRATION 1

/* PostgreSQL frontend headers - commented out for basic framework testing */
/* #include "postgres_fe.h" */

/* Database connection abstraction */
typedef struct PGconn PGconn;  /* Forward declaration for libpq connection */

/* Database connection status */
typedef enum {
    CONNECTION_OK = 0,
    CONNECTION_BAD,
    CONNECTION_STARTED,
    CONNECTION_MADE,
    CONNECTION_AWAITING_RESPONSE,
    CONNECTION_AUTH_OK,
    CONNECTION_SETENV,
    CONNECTION_SSL_STARTUP,
    CONNECTION_NEEDED
} ConnStatusType;

/* Include TECC module headers - using forward declarations to avoid header conflicts */
/* #include "../../frontend_parser/fe_adapter.h" */  /* Temporarily commented out due to header dependencies */
/* #include "../../tecc_processor/tecc_processor.h" */  /* Temporarily commented out due to header dependencies */
/* #include "../../tecc_libpq_integration/fe-tecc-integration.h" */  /* Temporarily commented out due to header dependencies */

/* Parser interface - simplified to avoid header conflicts */
typedef struct Node Node;
typedef struct List List;
typedef struct ParserError ParserError;

/* Simplified parser interface functions */
Node *tecc_parse_sql(const char *sql, ParserError **errors);
void tecc_parser_free_tree_node(Node *parse_tree);

/* Forward declarations for TECC types */
typedef struct Node Node;
typedef struct List List;
typedef struct TeccStatementData TeccStatementData;

/* Define TeccProcessorResult if not already defined */
#ifndef TECC_PROCESSOR_RESULT_DEFINED
typedef enum {
    TECC_PROCESSOR_RESULT_SUCCESS = 0,
    TECC_PROCESSOR_RESULT_ERROR = 1,
    TECC_PROCESSOR_RESULT_SKIP = 2
} TeccProcessorResult;
#define TECC_PROCESSOR_RESULT_DEFINED
#endif

/* Log levels */
typedef enum {
    LOG_LEVEL_ERROR = 0,    /* Error information */
    LOG_LEVEL_WARNING,      /* Warning information */
    LOG_LEVEL_INFO,         /* General information */
    LOG_LEVEL_DEBUG,        /* Debug information */
    LOG_LEVEL_TRACE         /* Detailed trace */
} LogLevel;

/* Test result codes */
typedef enum {
    TEST_RESULT_PASS = 0,   /* Test passed */
    TEST_RESULT_FAIL,       /* Test failed */
    TEST_RESULT_SKIP,       /* Test skipped */
    TEST_RESULT_ERROR       /* Test error */
} TestResult;

/* Test case structure */
typedef struct {
    const char *name;       /* Test case name */
    const char *description; /* Test description */
    const char *sql;        /* SQL statement to test */
    TestResult result;      /* Test result */
    double execution_time;  /* Execution time in seconds */
    char *error_message;    /* Error message if failed */
} TestCase;

/* Test suite structure */
typedef struct {
    const char *name;       /* Test suite name */
    TestCase *test_cases;   /* Array of test cases */
    int test_case_count;    /* Number of test cases */
    int passed_count;       /* Number of passed tests */
    int failed_count;       /* Number of failed tests */
    int skipped_count;      /* Number of skipped tests */
    double total_time;      /* Total execution time */
} TestSuite;

/* Test configuration structure */
typedef struct {
    LogLevel log_level;     /* Log level for test execution */
    bool verbose;           /* Verbose output */
    bool stop_on_error;     /* Stop on first error */
    bool enable_tracing;    /* Enable function tracing */
    const char *output_file; /* Output file for results */
} TestConfig;

/* Test database configuration */
typedef struct {
    const char *host;       /* Database host */
    const char *port;       /* Database port */
    const char *dbname;     /* Database name */
    const char *user;       /* Database user */
    const char *password;   /* Database password */
} TestDBConfig;

/* Performance metrics */
typedef struct {
    size_t parse_time_ms;      /* Parsing time in milliseconds */
    size_t process_time_ms;    /* Processing time in milliseconds */
    size_t memory_used_kb;     /* Memory usage in kilobytes */
    int sql_statements;        /* Number of SQL statements */
    int encrypted_columns;     /* Number of encrypted columns */
    int cache_hits;            /* Cache hits */
    int cache_misses;          /* Cache misses */
} PerformanceMetrics;

/* Function declarations */

/* Logging functions */
void test_log(LogLevel level, const char *module, const char *function, const char *format, ...);
void test_set_log_level(LogLevel level);
LogLevel test_get_log_level(void);

/* Test case management */
TestCase *test_case_create(const char *name, const char *description, const char *sql);
void test_case_free(TestCase *test_case);
void test_case_execute(TestCase *test_case, PGconn *conn);

/* Test suite management */
TestSuite *test_suite_create(const char *name);
void test_suite_free(TestSuite *suite);
void test_suite_add_case(TestSuite *suite, TestCase *test_case);
void test_suite_execute(TestSuite *suite, PGconn *conn);
void test_suite_print_summary(const TestSuite *suite);

/* Database connection */
PGconn *test_db_connect(const TestDBConfig *config);
void test_db_disconnect(PGconn *conn);
bool test_db_prepare_test_data(PGconn *conn);

/* Parser interface functions */
Node *test_parse_sql(const char *sql, LogLevel log_level);
void test_print_parse_tree(const Node *parse_tree, int indent);
bool test_validate_encrypted_columns(const Node *parse_tree,
                                     const char **encrypted_columns,
                                     int column_count);

/* Processor interface functions */
TeccStatementData *test_init_processor_context(PGconn *conn);
TeccProcessorResult test_process_statement(TeccStatementData *stmt_data,
                                           const Node *parse_tree);
void test_trace_processor_execution(TeccStatementData *stmt_data);
bool test_validate_rewritten_sql(const char *original_sql,
                                 const char *rewritten_sql);

/* Integration functions */
TestResult test_process_sql_statement(PGconn *conn, const char *sql);
void test_step_by_step_execution(PGconn *conn, const char *sql);
bool test_verify_scheduler_calls(TeccStatementData *stmt_data);
bool test_verify_cache_calls(TeccStatementData *stmt_data);

/* Performance monitoring */
void test_performance_start_timer(void);
double test_performance_stop_timer(void);
void test_performance_record_metrics(PerformanceMetrics *metrics);
void test_performance_print_report(const PerformanceMetrics *metrics);

/* Utility functions */
const char *test_result_to_string(TestResult result);
const char *log_level_to_string(LogLevel level);
void test_print_banner(const char *title);
void test_print_separator(void);

#endif /* TEST_INTEGRATION_H */
/* End - ReqID:TECC4.0 */