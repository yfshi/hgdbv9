/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_formatter_log.h
 *
 * Abstract:
 *   Logging utilities for TECC data formatter module.
 *   Provides independent logging functions for the formatter module.
 *
 * Authored by <wangxp@highgo.com>, 20260412.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_formatter_log.h
 */

#ifndef TECC_FORMATTER_LOG_H
#define TECC_FORMATTER_LOG_H

#include <stdio.h>
#include <stdarg.h>

/* Log levels */
typedef enum {
    TECC_FORMATTER_LOG_LEVEL_DEBUG = 0,
    TECC_FORMATTER_LOG_LEVEL_INFO,
    TECC_FORMATTER_LOG_LEVEL_WARNING,
    TECC_FORMATTER_LOG_LEVEL_ERROR
} TeccFormatterLogLevel;

/* Default log level */
#ifndef TECC_FORMATTER_LOG_LEVEL
#define TECC_FORMATTER_LOG_LEVEL TECC_FORMATTER_LOG_LEVEL_WARNING
#endif

/*
 * tecc_formatter_log - Generic logging function
 *
 * Logs a message with the specified level and function name.
 *
 * @param level Log level
 * @param func_name Function name (use __func__)
 * @param format Format string (printf-style)
 * @param ... Arguments for format string
 */
static void
tecc_formatter_log(TeccFormatterLogLevel level,
                   const char *func_name,
                   const char *format,
                   ...)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    va_list args;
    const char *level_str;

    /* Check if logging is enabled for this level */
    if (level < TECC_FORMATTER_LOG_LEVEL)
        return;

    /* Format log message */
    va_start(args, format);
    switch (level) {
        case TECC_FORMATTER_LOG_LEVEL_DEBUG:
            level_str = "DEBUG";
            break;
        case TECC_FORMATTER_LOG_LEVEL_INFO:
            level_str = "INFO";
            break;
        case TECC_FORMATTER_LOG_LEVEL_WARNING:
            level_str = "WARNING";
            break;
        case TECC_FORMATTER_LOG_LEVEL_ERROR:
            level_str = "ERROR";
            break;
        default:
            level_str = "UNKNOWN";
            break;
    }

    /* Print to stderr */
    fprintf(stderr, "[TECC_FORMATTER %s] %s: ", level_str, func_name);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");

    va_end(args);
}

/*
 * Convenience macros for logging
 */
#define tecc_formatter_debug(format, ...) \
    tecc_formatter_log(TECC_FORMATTER_LOG_LEVEL_DEBUG, __func__, format, ##__VA_ARGS__)

#define tecc_formatter_info(format, ...) \
    tecc_formatter_log(TECC_FORMATTER_LOG_LEVEL_INFO, __func__, format, ##__VA_ARGS__)

#define tecc_formatter_warning(format, ...) \
    tecc_formatter_log(TECC_FORMATTER_LOG_LEVEL_WARNING, __func__, format, ##__VA_ARGS__)

#define tecc_formatter_error(format, ...) \
    tecc_formatter_log(TECC_FORMATTER_LOG_LEVEL_ERROR, __func__, format, ##__VA_ARGS__)

#endif /* TECC_FORMATTER_LOG_H */
/* End - ReqID:TECC4.0 */