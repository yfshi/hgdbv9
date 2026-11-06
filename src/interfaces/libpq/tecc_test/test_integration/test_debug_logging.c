/* Begin - ReqID:TECC4.0 */
/*
 * File: test_debug_logging.c
 * Abstract:
 *   Debug logging and tracing functionality for TECC integration testing.
 *   Provides detailed execution tracing, error recording, and performance statistics.
 *
 * Authored by <wangxp@highgo.com>, 20260415.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_test/test_integration/test_debug_logging.c
 */

#include "include/test_integration.h"
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

/* Global logging configuration */
static struct {
    LogLevel current_level;
    bool enable_tracing;
    bool log_to_file;
    FILE *log_file;
    int indent_level;
} logging_config = {
    .current_level = LOG_LEVEL_INFO,
    .enable_tracing = false,
    .log_to_file = false,
    .log_file = NULL,
    .indent_level = 0
};

/* Function call stack for tracing */
#define MAX_CALL_STACK 50
static struct {
    const char *module;
    const char *function;
    struct timeval entry_time;
} call_stack[MAX_CALL_STACK];
static int call_stack_top = -1;

/* Performance statistics */
static struct {
    unsigned long function_calls;
    unsigned long parse_calls;
    unsigned long process_calls;
    unsigned long cache_hits;
    unsigned long cache_misses;
    unsigned long errors;
    struct timeval total_time;
} perf_stats = {0};

/* Initialize logging system */
void test_logging_init(LogLevel level, bool enable_tracing, const char *log_file_path)
{
    logging_config.current_level = level;
    logging_config.enable_tracing = enable_tracing;
    logging_config.indent_level = 0;

    if (log_file_path) {
        logging_config.log_file = fopen(log_file_path, "a");
        if (logging_config.log_file) {
            logging_config.log_to_file = true;
            test_log(LOG_LEVEL_INFO, "LOGGING", "test_logging_init",
                     "Logging to file: %s", log_file_path);
        } else {
            test_log(LOG_LEVEL_ERROR, "LOGGING", "test_logging_init",
                     "Failed to open log file: %s", log_file_path);
        }
    }

    /* Initialize performance statistics */
    gettimeofday(&perf_stats.total_time, NULL);
}

/* Cleanup logging system */
void test_logging_cleanup(void)
{
    if (logging_config.log_file) {
        fclose(logging_config.log_file);
        logging_config.log_file = NULL;
        logging_config.log_to_file = false;
    }
}

/* Get current timestamp string */
static void get_timestamp(char *buffer, size_t buffer_size)
{
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    struct timeval tv;
    gettimeofday(&tv, NULL);

    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", tm_info);

    /* Append milliseconds */
    char ms_buffer[10];
    snprintf(ms_buffer, sizeof(ms_buffer), ".%03ld", tv.tv_usec / 1000);
    strncat(buffer, ms_buffer, buffer_size - strlen(buffer) - 1);
}

/* Get indentation string for current call stack depth */
static const char *get_indentation(void)
{
    static char indent[256] = "";

    if (logging_config.indent_level <= 0) {
        return "";
    }

    int indent_len = logging_config.indent_level * 2;
    if (indent_len > 255) indent_len = 255;

    memset(indent, ' ', indent_len);
    indent[indent_len] = '\0';

    return indent;
}

/* Write log message to all outputs */
static void write_log_message(LogLevel level, const char *module, const char *function,
                              const char *format, va_list args)
{
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));

    const char *indent = get_indentation();

    /* Format the message */
    char message[1024];
    vsnprintf(message, sizeof(message), format, args);

    /* Format full log line */
    char log_line[2048];
    snprintf(log_line, sizeof(log_line), "[%s] [%s] [%s] [%s] - %s%s\n",
             timestamp,
             log_level_to_string(level),
             module,
             function,
             indent,
             message);

    /* Write to stdout */
    printf("%s", log_line);

    /* Write to log file if enabled */
    if (logging_config.log_to_file && logging_config.log_file) {
        fprintf(logging_config.log_file, "%s", log_line);
        fflush(logging_config.log_file);
    }
}

/* Main logging function - defined in test_integration_main.c */
/* void test_log(LogLevel level, const char *module, const char *function,
              const char *format, ...); */

/* Set log level - defined in test_integration_main.c */
/* void test_set_log_level(LogLevel level); */

/* Get current log level - defined in test_integration_main.c */
/* LogLevel test_get_log_level(void); */

/* Enable/disable tracing */
void test_set_tracing_enabled(bool enabled)
{
    logging_config.enable_tracing = enabled;
    test_log(LOG_LEVEL_INFO, "LOGGING", "test_set_tracing_enabled",
             "Function tracing %s", enabled ? "enabled" : "disabled");
}

/* Function call tracing */
void test_trace_function_entry(const char *module, const char *function, const char *params)
{
    if (!logging_config.enable_tracing || call_stack_top >= MAX_CALL_STACK - 1)
        return;

    call_stack_top++;
    call_stack[call_stack_top].module = module;
    call_stack[call_stack_top].function = function;
    gettimeofday(&call_stack[call_stack_top].entry_time, NULL);

    logging_config.indent_level = call_stack_top + 1;

    perf_stats.function_calls++;

    test_log(LOG_LEVEL_TRACE, module, function, "ENTER%s%s",
             params ? " (" : "",
             params ? params : "",
             params ? ")" : "");
}

void test_trace_function_exit(const char *module, const char *function, const char *result)
{
    if (!logging_config.enable_tracing || call_stack_top < 0)
        return;

    /* Calculate execution time */
    struct timeval exit_time;
    gettimeofday(&exit_time, NULL);

    long elapsed_ms = (exit_time.tv_sec - call_stack[call_stack_top].entry_time.tv_sec) * 1000 +
                      (exit_time.tv_usec - call_stack[call_stack_top].entry_time.tv_usec) / 1000;

    test_log(LOG_LEVEL_TRACE, module, function, "EXIT (%ld ms)%s%s",
             elapsed_ms,
             result ? " -> " : "",
             result ? result : "");

    call_stack_top--;
    logging_config.indent_level = call_stack_top + 1;
}

/* Data flow tracing */
void test_trace_data_flow(const char *from_module, const char *to_module,
                          const char *data_type, const void *data)
{
    if (!logging_config.enable_tracing)
        return;

    test_log(LOG_LEVEL_TRACE, "DATAFLOW", "trace",
             "%s -> %s: %s%s%s",
             from_module,
             to_module,
             data_type,
             data ? " = " : "",
             data ? (const char *)data : "");
}

/* Module-specific tracing functions */

/* Parser tracing */
void test_trace_parser_entry(const char *function, const char *sql)
{
    test_trace_function_entry("PARSER", function, sql);
    perf_stats.parse_calls++;
}

void test_trace_parser_exit(const char *function, const Node *parse_tree)
{
    const char *result = parse_tree ? "ParseTree" : "NULL";
    test_trace_function_exit("PARSER", function, result);
}

/* Processor tracing */
void test_trace_processor_entry(const char *function, TeccStatementData *stmt_data)
{
    char params[256];
    if (stmt_data) {
        snprintf(params, sizeof(params), "stmt_data=%p", (void *)stmt_data);
    } else {
        snprintf(params, sizeof(params), "stmt_data=NULL");
    }
    test_trace_function_entry("PROCESSOR", function, params);
    perf_stats.process_calls++;
}

void test_trace_processor_exit(const char *function, TeccProcessorResult result)
{
    char result_str[64];
    snprintf(result_str, sizeof(result_str), "Result=%d", result);
    test_trace_function_exit("PROCESSOR", function, result_str);
}

/* Cache tracing */
void test_trace_cache_access(const char *operation, const char *key, bool hit)
{
    if (hit) {
        perf_stats.cache_hits++;
        test_log(LOG_LEVEL_DEBUG, "CACHE", operation,
                 "Cache HIT: %s", key);
    } else {
        perf_stats.cache_misses++;
        test_log(LOG_LEVEL_DEBUG, "CACHE", operation,
                 "Cache MISS: %s", key);
    }
}

/* Error recording */
void test_record_error(const char *module, const char *function,
                       int error_code, const char *error_message)
{
    perf_stats.errors++;

    test_log(LOG_LEVEL_ERROR, module, function,
             "Error %d: %s", error_code, error_message);

    /* Log call stack if tracing is enabled */
    if (logging_config.enable_tracing && call_stack_top >= 0) {
        test_log(LOG_LEVEL_ERROR, "STACK", "trace",
                 "Call stack at error:");

        int i;
        for (i = call_stack_top; i >= 0; i--) {
            test_log(LOG_LEVEL_ERROR, "STACK", "trace",
                     "  [%d] %s::%s", i, call_stack[i].module, call_stack[i].function);
        }
    }
}

/* Performance statistics */

/* Start performance measurement */
void test_perf_start(const char *operation)
{
    test_trace_function_entry("PERF", operation, NULL);
}

/* Stop performance measurement and return elapsed time in milliseconds */
long test_perf_stop(const char *operation)
{
    struct timeval exit_time;
    gettimeofday(&exit_time, NULL);

    if (call_stack_top >= 0 &&
        strcmp(call_stack[call_stack_top].module, "PERF") == 0 &&
        strcmp(call_stack[call_stack_top].function, operation) == 0) {

        long elapsed_ms = (exit_time.tv_sec - call_stack[call_stack_top].entry_time.tv_sec) * 1000 +
                          (exit_time.tv_usec - call_stack[call_stack_top].entry_time.tv_usec) / 1000;

        test_trace_function_exit("PERF", operation, NULL);

        test_log(LOG_LEVEL_DEBUG, "PERF", operation,
                 "Completed in %ld ms", elapsed_ms);

        return elapsed_ms;
    }

    return 0;
}

/* Get performance statistics */
void test_perf_get_stats(unsigned long *function_calls,
                         unsigned long *parse_calls,
                         unsigned long *process_calls,
                         unsigned long *cache_hits,
                         unsigned long *cache_misses,
                         unsigned long *errors,
                         long *total_time_ms)
{
    if (function_calls) *function_calls = perf_stats.function_calls;
    if (parse_calls) *parse_calls = perf_stats.parse_calls;
    if (process_calls) *process_calls = perf_stats.process_calls;
    if (cache_hits) *cache_hits = perf_stats.cache_hits;
    if (cache_misses) *cache_misses = perf_stats.cache_misses;
    if (errors) *errors = perf_stats.errors;

    if (total_time_ms) {
        struct timeval now;
        gettimeofday(&now, NULL);

        *total_time_ms = (now.tv_sec - perf_stats.total_time.tv_sec) * 1000 +
                         (now.tv_usec - perf_stats.total_time.tv_usec) / 1000;
    }
}

/* Print performance report */
void test_perf_print_report(void)
{
    long total_time_ms;
    test_perf_get_stats(NULL, NULL, NULL, NULL, NULL, NULL, &total_time_ms);

    test_print_separator();
    test_log(LOG_LEVEL_INFO, "PERF", "test_perf_print_report",
             "Performance Statistics");
    test_log(LOG_LEVEL_INFO, "PERF", "test_perf_print_report",
             "Total time: %ld ms", total_time_ms);
    test_log(LOG_LEVEL_INFO, "PERF", "test_perf_print_report",
             "Function calls: %lu", perf_stats.function_calls);
    test_log(LOG_LEVEL_INFO, "PERF", "test_perf_print_report",
             "Parse calls: %lu", perf_stats.parse_calls);
    test_log(LOG_LEVEL_INFO, "PERF", "test_perf_print_report",
             "Process calls: %lu", perf_stats.process_calls);
    test_log(LOG_LEVEL_INFO, "PERF", "test_perf_print_report",
             "Cache hits: %lu, misses: %lu (hit rate: %.1f%%)",
             perf_stats.cache_hits, perf_stats.cache_misses,
             perf_stats.cache_hits + perf_stats.cache_misses > 0 ?
             (float)perf_stats.cache_hits / (perf_stats.cache_hits + perf_stats.cache_misses) * 100 : 0.0);
    test_log(LOG_LEVEL_INFO, "PERF", "test_perf_print_report",
             "Errors: %lu", perf_stats.errors);
    test_print_separator();
}

/* Reset performance statistics */
void test_perf_reset_stats(void)
{
    memset(&perf_stats, 0, sizeof(perf_stats));
    gettimeofday(&perf_stats.total_time, NULL);

    test_log(LOG_LEVEL_INFO, "PERF", "test_perf_reset_stats",
             "Performance statistics reset");
}

/* Memory usage tracking */
void test_trace_memory_allocation(const char *operation, size_t size, void *ptr)
{
    test_log(LOG_LEVEL_TRACE, "MEMORY", operation,
             "Allocated %zu bytes at %p", size, ptr);
}

void test_trace_memory_free(const char *operation, void *ptr)
{
    test_log(LOG_LEVEL_TRACE, "MEMORY", operation,
             "Freed memory at %p", ptr);
}

/* Execution step tracing */
void test_trace_execution_step(int step, const char *description, const char *details)
{
    test_log(LOG_LEVEL_INFO, "EXECUTION", "step",
             "Step %d: %s%s%s",
             step,
             description,
             details ? " - " : "",
             details ? details : "");
}

/* Module initialization tracing */
void test_trace_module_init(const char *module, bool success, const char *message)
{
    if (success) {
        test_log(LOG_LEVEL_INFO, module, "init",
                 "Initialized successfully%s%s",
                 message ? ": " : "",
                 message ? message : "");
    } else {
        test_log(LOG_LEVEL_ERROR, module, "init",
                 "Initialization failed%s%s",
                 message ? ": " : "",
                 message ? message : "");
    }
}

/* Module cleanup tracing */
void test_trace_module_cleanup(const char *module, bool success, const char *message)
{
    if (success) {
        test_log(LOG_LEVEL_INFO, module, "cleanup",
                 "Cleaned up successfully%s%s",
                 message ? ": " : "",
                 message ? message : "");
    } else {
        test_log(LOG_LEVEL_WARNING, module, "cleanup",
                 "Cleanup failed%s%s",
                 message ? ": " : "",
                 message ? message : "");
    }
}

/* Test case execution tracing */
void test_trace_test_case_start(const char *test_name, const char *sql)
{
    test_log(LOG_LEVEL_INFO, "TESTCASE", "start",
             "Starting test: %s", test_name);
    test_log(LOG_LEVEL_DEBUG, "TESTCASE", "start",
             "SQL: %s", sql);
}

void test_trace_test_case_end(const char *test_name, TestResult result, double execution_time)
{
    test_log(LOG_LEVEL_INFO, "TESTCASE", "end",
             "Test %s completed: %s (%.3f seconds)",
             test_name, test_result_to_string(result), execution_time);
}

/* SQL statement tracing */
void test_trace_sql_statement(const char *operation, const char *sql, const char *details)
{
    test_log(LOG_LEVEL_DEBUG, "SQL", operation,
             "%s%s%s",
             sql,
             details ? " - " : "",
             details ? details : "");
}

/* Encrypted column detection tracing */
void test_trace_encrypted_column_detection(const char *table_name, const char *column_name,
                                           bool is_encrypted, const char *algorithm)
{
    if (is_encrypted) {
        test_log(LOG_LEVEL_INFO, "ENCRYPTION", "detect",
                 "Encrypted column detected: %s.%s (%s)",
                 table_name, column_name, algorithm ? algorithm : "unknown");
    } else {
        test_log(LOG_LEVEL_DEBUG, "ENCRYPTION", "detect",
                 "Plain column: %s.%s", table_name, column_name);
    }
}

/* SQL rewrite tracing */
void test_trace_sql_rewrite(const char *original, const char *rewritten)
{
    test_log(LOG_LEVEL_INFO, "REWRITE", "process",
             "Original SQL: %s", original);
    test_log(LOG_LEVEL_INFO, "REWRITE", "process",
             "Rewritten SQL: %s", rewritten);

    if (original && rewritten && strcmp(original, rewritten) != 0) {
        test_log(LOG_LEVEL_DEBUG, "REWRITE", "process",
                 "SQL was modified for encryption");
    } else {
        test_log(LOG_LEVEL_DEBUG, "REWRITE", "process",
                 "SQL unchanged (no encryption needed)");
    }
}
/* End - ReqID:TECC4.0 */