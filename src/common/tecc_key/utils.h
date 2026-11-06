/*-------------------------------------------------------------------------
 *
 * utils.h
 *    Utility functions for TECC key management module
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/common/tecc_key/utils.h
 *
 * ReqID:TECC4.0 - Key management utilities
 * Authored by <wanghaoyan@highgo.com>, 20260408.
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_KEY_UTILS_H
#define TECC_KEY_UTILS_H

#include "postgres.h"
#include "c.h"
#include "port/pg_bswap.h"
#include "key_manager.h"

/* Error codes */
typedef enum TeccKeyErrorCode
{
    TECC_KEY_SUCCESS = 0,
    TECC_KEY_ERROR_INVALID_PARAM = 1000,
    TECC_KEY_ERROR_OUT_OF_MEMORY = 1001,
    TECC_KEY_ERROR_KEY_NOT_FOUND = 1100,
    TECC_KEY_ERROR_DECRYPT_FAILED = 1101,
    TECC_KEY_ERROR_PERMISSION_DENIED = 1200,
    TECC_KEY_ERROR_KMS_CONNECTION_FAILED = 1300,
    TECC_KEY_ERROR_DATABASE_ERROR = 1400,
    TECC_KEY_ERROR_UNSUPPORTED_ALGORITHM = 1500,
    TECC_KEY_ERROR_CACHE_FULL = 1600,
    TECC_KEY_ERROR_CONCURRENT_ACCESS = 1700,
    TECC_KEY_ERROR_CONFIG_ERROR = 1800,
    TECC_KEY_ERROR_INTERNAL = 9999
} TeccKeyErrorCode;

/* Error structure */
typedef struct TeccKeyError
{
    TeccKeyErrorCode error_code;    /* Error code */
    char *error_message;            /* Error message */
    char *function_name;            /* Function name where error occurred */
    char *file_name;                /* File name */
    int line_number;                /* Line number */
    struct TeccKeyError *cause;     /* Cause error (for error chaining) */
} TeccKeyError;

/* Log levels */
typedef enum LogLevel
{
    LOG_LEVEL_DEBUG = 14,    /* DEBUG1 - Debug information */
    LOG_LEVEL_INFO = 17,     /* INFO - Informational messages */
    LOG_LEVEL_WARNING = 19,  /* WARNING - Warning messages */
    LOG_LEVEL_ERROR = 21,    /* ERROR - Error messages */
    LOG_LEVEL_FATAL = 22     /* FATAL - Fatal errors */
} LogLevel;

/* Log entry */
typedef struct LogEntry
{
    LogLevel level;                 /* Log level */
    char *message;                  /* Log message */
    char *function_name;            /* Function name */
    char *file_name;                /* File name */
    int line_number;                /* Line number */
    time_t timestamp;               /* Timestamp */
    pid_t process_id;               /* Process ID */
    pthread_t thread_id;            /* Thread ID */
} LogEntry;

/* Performance metric types */
typedef enum MetricType
{
    METRIC_COUNTER,     /* Counter metric (increments) */
    METRIC_GAUGE,       /* Gauge metric (current value) */
    METRIC_HISTOGRAM,   /* Histogram metric (distribution) */
    METRIC_TIMER        /* Timer metric (duration) */
} MetricType;

/* Performance metric */
typedef struct PerformanceMetric
{
    char *name;                     /* Metric name */
    MetricType type;                /* Metric type */
    union {
        uint64_t counter_value;     /* Counter value */
        double gauge_value;         /* Gauge value */
        struct {
            uint64_t count;         /* Histogram count */
            double sum;             /* Histogram sum */
            double min;             /* Histogram min */
            double max;             /* Histogram max */
        } histogram;
        struct {
            uint64_t count;         /* Timer count */
            double total_time;      /* Total time in seconds */
        } timer;
    } value;
    time_t last_update;             /* Last update time */
    struct PerformanceMetric *next; /* Next metric in list */
} PerformanceMetric;

/* Debug context */
typedef struct DebugContext
{
    bool enabled;                   /* Whether debugging is enabled */
    int trace_level;                /* Trace level (0-10) */
    char *trace_filter;             /* Trace filter (function names) */
    FILE *trace_file;               /* Trace output file */
    bool memory_tracking;           /* Whether memory tracking is enabled */
} DebugContext;

/* Function prototypes */

/* Error handling */
extern TeccKeyError *tecc_key_error_create(TeccKeyErrorCode error_code,
                                          const char *message,
                                          const char *function_name,
                                          const char *file_name,
                                          int line_number);
extern void tecc_key_error_destroy(TeccKeyError *error);
extern TeccKeyError *tecc_key_error_with_cause(TeccKeyErrorCode error_code,
                                              const char *message,
                                              const char *function_name,
                                              const char *file_name,
                                              int line_number,
                                              TeccKeyError *cause);
extern const char *tecc_key_error_to_string(const TeccKeyError *error);
extern char *tecc_key_error_to_json(const TeccKeyError *error);
extern void tecc_key_error_log(const TeccKeyError *error, LogLevel level);

/* Logging */
extern void tecc_key_log_init(const char *log_file, LogLevel min_level,
                             bool log_to_stdout);
extern void tecc_key_log_cleanup(void);
extern bool tecc_key_is_debug_enabled(void);
extern void tecc_key_log(LogLevel level, const char *function_name,
                        const char *file_name, int line_number,
                        const char *format, ...)
pg_attribute_printf(5, 6);
extern void tecc_key_log_hexdump(LogLevel level, const char *label,
                                const void *data, size_t length);
extern int tecc_key_log_rotate(void);
extern LogLevel tecc_key_log_get_level(void);
extern void tecc_key_log_set_level(LogLevel level);

/* Performance monitoring */
extern void tecc_key_metrics_init(void);
extern void tecc_key_metrics_cleanup(void);
extern PerformanceMetric *tecc_key_metric_create(const char *name, MetricType type);
extern void tecc_key_metric_destroy(PerformanceMetric *metric);
extern void tecc_key_metric_increment(const char *name, uint64_t value);
extern void tecc_key_metric_gauge_set(const char *name, double value);
extern void tecc_key_metric_histogram_update(const char *name, double value);
extern void tecc_key_metric_timer_start(const char *name);
extern void tecc_key_metric_timer_stop(const char *name);
extern char *tecc_key_metrics_to_json(void);
extern void tecc_key_metrics_reset(void);

/* Memory utilities */
extern void *tecc_key_malloc(size_t size, const char *function_name,
                            const char *file_name, int line_number);
extern void *tecc_key_calloc(size_t nmemb, size_t size,
                            const char *function_name,
                            const char *file_name, int line_number);
extern void *tecc_key_realloc(void *ptr, size_t size,
                             const char *function_name,
                             const char *file_name, int line_number);
extern void tecc_key_free(void *ptr);
extern void tecc_key_memory_report(void);
extern size_t tecc_key_memory_usage(void);

/* String utilities */
extern char *tecc_key_strdup(const char *str);
extern char *tecc_key_strndup(const char *str, size_t n);
extern char *tecc_key_vasprintf(const char *format, va_list args);
extern char *tecc_key_asprintf(const char *format, ...)
pg_attribute_printf(1, 2);
extern char **tecc_key_strsplit(const char *str, const char *delim, int *count);
extern void tecc_key_strfreev(char **str_array);
extern bool tecc_key_str_equals(const char *a, const char *b);
extern bool tecc_key_str_equals_ignore_case(const char *a, const char *b);
extern char *tecc_key_str_trim(char *str);
extern char *tecc_key_str_escape(const char *str);

/* Hex and base64 encoding */
extern char *tecc_key_bytes_to_hex(const unsigned char *data, size_t length);
extern unsigned char *tecc_key_hex_to_bytes(const char *hex, size_t *length);
extern char *tecc_key_bytes_to_base64(const unsigned char *data, size_t length);
extern unsigned char *tecc_key_base64_to_bytes(const char *base64, size_t *length);

/* Time utilities */
extern time_t tecc_key_current_time(void);
extern char *tecc_key_format_time(time_t time_val);
extern char *tecc_key_format_timestamp(TimestampTz timestamp);
extern time_t tecc_key_parse_time(const char *time_str);
extern int tecc_key_time_diff_ms(time_t start, time_t end);

/* Debug utilities */
extern DebugContext *tecc_key_debug_context_create(void);
extern void tecc_key_debug_context_destroy(DebugContext *context);
extern void tecc_key_trace(int level, const char *function_name,
                          const char *file_name, int line_number,
                          const char *format, ...)
pg_attribute_printf(5, 6);
extern void tecc_key_assert(bool condition, const char *expression,
                           const char *function_name,
                           const char *file_name, int line_number);
extern void tecc_key_dump_stacktrace(void);

/* Configuration utilities */
extern bool tecc_key_parse_bool(const char *value);
extern int tecc_key_parse_int(const char *value, int default_value);
extern char *tecc_key_parse_string(const char *value);
extern char **tecc_key_parse_list(const char *value, const char *delim, int *count);

/* Validation utilities */
extern bool tecc_key_validate_key_name(const char *key_name);
extern bool tecc_key_validate_oid(Oid oid);
extern bool tecc_key_validate_timestamp(TimestampTz timestamp);
extern bool tecc_key_validate_key_length(int key_length);

/* Conversion utilities */
extern const char *tecc_key_algorithm_type_to_string(TeccAlgorithmType algo_type);
extern TeccAlgorithmType tecc_key_string_to_algorithm_type(const char *algo_str);
extern const char *tecc_key_kms_type_to_string(KmsType kms_type);
extern KmsType tecc_key_string_to_kms_type(const char *kms_str);

/* Macro definitions for convenience */
#define TECC_KEY_MALLOC(size) \
    tecc_key_malloc((size), __func__, __FILE__, __LINE__)

#define TECC_KEY_CALLOC(nmemb, size) \
    tecc_key_calloc((nmemb), (size), __func__, __FILE__, __LINE__)

#define TECC_KEY_REALLOC(ptr, size) \
    tecc_key_realloc((ptr), (size), __func__, __FILE__, __LINE__)

#define TECC_KEY_FREE(ptr) \
    do { if (ptr) { tecc_key_free(ptr); (ptr) = NULL; } } while (0)

#define TECC_KEY_STRDUP(str) \
    tecc_key_strdup(str)

#define TECC_KEY_ERROR(code, msg) \
    tecc_key_error_create((code), (msg), __func__, __FILE__, __LINE__)

#define TECC_KEY_LOG(level, ...) \
    tecc_key_log((level), __func__, __FILE__, __LINE__, __VA_ARGS__)

#define TECC_KEY_TRACE(level, ...) \
    tecc_key_trace((level), __func__, __FILE__, __LINE__, __VA_ARGS__)

#define TECC_KEY_ASSERT(condition) \
    tecc_key_assert((condition), #condition, __func__, __FILE__, __LINE__)

#endif /* TECC_KEY_UTILS_H */