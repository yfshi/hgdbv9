/*-------------------------------------------------------------------------
 *
 * utils.c
 *    Utility functions implementation for TECC key management module
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/common/tecc_key/utils.c
 *
 * ReqID:TECC4.0 - Key management utilities
 * Authored by <wanghaoyan@highgo.com>, 20260408.
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *-------------------------------------------------------------------------
 */
#include "postgres_fe.h"
#include "c.h"
#include "common/fe_memutils.h"
#include "port/pg_bswap.h"
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include "utils.h"

/* Global logging configuration */
static struct {
    LogLevel min_level;
    char *log_file;
    FILE *log_fp;
    bool log_to_stdout;
    pthread_mutex_t log_lock;
} logging_config = {
    .min_level = LOG_LEVEL_DEBUG,
    .log_file = NULL,
    .log_fp = NULL,
    .log_to_stdout = false,
    .log_lock = PTHREAD_MUTEX_INITIALIZER
};


/*
 * tecc_key_error_create
 *    Create a new error structure
 */
TeccKeyError *
tecc_key_error_create(TeccKeyErrorCode error_code,
                     const char *message,
                     const char *function_name,
                     const char *file_name,
                     int line_number)
{
    TeccKeyError *error = NULL;

    error = (TeccKeyError *)pg_malloc0(sizeof(TeccKeyError));
    if (error == NULL)
        return NULL;

    error->error_code = error_code;

    if (message != NULL)
        error->error_message = pg_strdup(message);
    else
        error->error_message = NULL;

    if (function_name != NULL)
        error->function_name = pg_strdup(function_name);
    else
        error->function_name = NULL;

    if (file_name != NULL)
        error->file_name = pg_strdup(file_name);
    else
        error->file_name = NULL;

    error->line_number = line_number;
    error->cause = NULL;

    return error;
}

/*
 * tecc_key_error_destroy
 *    Destroy an error structure
 */
void
tecc_key_error_destroy(TeccKeyError *error)
{
    if (error == NULL)
        return;

    if (error->error_message != NULL)
        pg_free(error->error_message);

    if (error->function_name != NULL)
        pg_free(error->function_name);

    if (error->file_name != NULL)
        pg_free(error->file_name);

    /* Destroy cause error recursively */
    if (error->cause != NULL)
        tecc_key_error_destroy(error->cause);

    pg_free(error);
}

/*
 * tecc_key_error_with_cause
 *    Create an error structure with a cause
 */
TeccKeyError *
tecc_key_error_with_cause(TeccKeyErrorCode error_code,
                         const char *message,
                         const char *function_name,
                         const char *file_name,
                         int line_number,
                         TeccKeyError *cause)
{
    TeccKeyError *error = tecc_key_error_create(error_code, message,
                                               function_name, file_name,
                                               line_number);
    if (error != NULL)
        error->cause = cause;

    return error;
}

/*
 * tecc_key_error_to_string
 *    Convert error to string representation
 */
const char *
tecc_key_error_to_string(const TeccKeyError *error)
{
    static char buffer[1024];
    int len = 0;

    if (error == NULL)
        return "NULL error";

    len = snprintf(buffer, sizeof(buffer),
                  "Error %d at %s:%d in %s(): %s",
                  error->error_code,
                  error->file_name ? error->file_name : "unknown",
                  error->line_number,
                  error->function_name ? error->function_name : "unknown",
                  error->error_message ? error->error_message : "no message");

    if (error->cause != NULL && len < (int)sizeof(buffer) - 100)
    {
        snprintf(buffer + len, sizeof(buffer) - len,
                " (caused by: %s)",
                tecc_key_error_to_string(error->cause));
    }

    return buffer;
}

/*
 * tecc_key_error_to_json
 *    Convert error to JSON representation
 */
char *
tecc_key_error_to_json(const TeccKeyError *error)
{
    char *json = NULL;
    char *cause_json = NULL;

    if (error == NULL)
        return pg_strdup("null");

    if (error->cause != NULL)
        cause_json = tecc_key_error_to_json(error->cause);

    json = psprintf(
        "{\"error_code\": %d, "
        "\"error_message\": \"%s\", "
        "\"function_name\": \"%s\", "
        "\"file_name\": \"%s\", "
        "\"line_number\": %d, "
        "\"cause\": %s}",
        error->error_code,
        error->error_message ? error->error_message : "",
        error->function_name ? error->function_name : "",
        error->file_name ? error->file_name : "",
        error->line_number,
        cause_json ? cause_json : "null");

    if (cause_json != NULL)
        pg_free(cause_json);

    return json;
}

/*
 * tecc_key_error_log
 *    Log an error
 */
void
tecc_key_error_log(const TeccKeyError *error, LogLevel level)
{
    if (error == NULL)
        return;

    tecc_key_log(level, error->function_name, error->file_name,
                error->line_number, "Error %d: %s",
                error->error_code, error->error_message);

    if (error->cause != NULL)
        tecc_key_error_log(error->cause, level);
}

/*
 * tecc_key_log_init
 *    Initialize logging system
 */
void
tecc_key_log_init(const char *log_file, LogLevel min_level,
                 bool log_to_stdout)
{
    pthread_mutex_lock(&logging_config.log_lock);

    /* Close existing log file if open */
    if (logging_config.log_fp != NULL && logging_config.log_fp != stdout)
    {
        fclose(logging_config.log_fp);
        logging_config.log_fp = NULL;
    }

    /* Free existing log file path */
    if (logging_config.log_file != NULL)
    {
        pg_free(logging_config.log_file);
        logging_config.log_file = NULL;
    }

    /* Set new configuration */
    logging_config.min_level = min_level;
    logging_config.log_to_stdout = log_to_stdout;

    if (log_file != NULL)
    {
        logging_config.log_file = pg_strdup(log_file);
        if (logging_config.log_file != NULL)
        {
            /* Open log file for appending */
            logging_config.log_fp = fopen(log_file, "a");
            if (logging_config.log_fp == NULL)
            {
                /* Fall back to stderr */
                logging_config.log_fp = stderr;
                tecc_key_log(LOG_LEVEL_ERROR, __func__, __FILE__, __LINE__,
                            "Failed to open log file %s: %s", log_file, strerror(errno));
            }
        }
    }

    /* If no log file or open failed, use stderr */
    if (logging_config.log_fp == NULL)
        logging_config.log_fp = stderr;

    pthread_mutex_unlock(&logging_config.log_lock);
}

/*
 * tecc_key_log_cleanup
 *    Cleanup logging system
 */
void
tecc_key_log_cleanup(void)
{
    pthread_mutex_lock(&logging_config.log_lock);

    if (logging_config.log_fp != NULL && logging_config.log_fp != stdout &&
        logging_config.log_fp != stderr)
    {
        fclose(logging_config.log_fp);
    }

    if (logging_config.log_file != NULL)
        pg_free(logging_config.log_file);

    logging_config.log_fp = NULL;
    logging_config.log_file = NULL;
    logging_config.min_level = LOG_LEVEL_INFO;
    logging_config.log_to_stdout = false;

    pthread_mutex_unlock(&logging_config.log_lock);
}

/*
 * tecc_key_is_debug_enabled
 *    Check if DEBUG level logging is enabled
 */
bool
tecc_key_is_debug_enabled(void)
{
    return logging_config.min_level <= LOG_LEVEL_DEBUG;
}

/*
 * tecc_key_log
 *    Log a message
 */
void
tecc_key_log(LogLevel level, const char *function_name,
            const char *file_name, int line_number,
            const char *format, ...)
{
    va_list args;
    char message[1024];
    char timestamp[64];
    time_t now;
    struct tm *tm_info;


    /* Check if we should log this message */
    if (level < logging_config.min_level)
        return;

    /* Get current time */
    now = time(NULL);
    tm_info = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    /* Format message */
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    pthread_mutex_lock(&logging_config.log_lock);

    /* If log_fp is NULL, use stderr for debugging */
    if (logging_config.log_fp == NULL)
        logging_config.log_fp = stderr;

    /* Log to file */
    if (logging_config.log_fp != NULL)
    {
        const char *level_str = NULL;
        switch (level)
        {
            case LOG_LEVEL_DEBUG:   level_str = "DEBUG"; break;
            case LOG_LEVEL_INFO:    level_str = "INFO"; break;
            case LOG_LEVEL_WARNING: level_str = "WARNING"; break;
            case LOG_LEVEL_ERROR:   level_str = "ERROR"; break;
            case LOG_LEVEL_FATAL:   level_str = "FATAL"; break;
            default:                level_str = "UNKNOWN"; break;
        }

        fprintf(logging_config.log_fp, "[%s] [%s] [%s:%d %s] %s\n",
                timestamp, level_str,
                file_name ? file_name : "unknown",
                line_number,
                function_name ? function_name : "unknown",
                message);

        fflush(logging_config.log_fp);
    }

    /* Log to stdout if configured */
    if (logging_config.log_to_stdout && logging_config.log_fp != stdout)
    {
        fprintf(stdout, "[%s] [%s] [%s:%d %s] %s\n",
                timestamp, "INFO",
                file_name ? file_name : "unknown",
                line_number,
                function_name ? function_name : "unknown",
                message);
        fflush(stdout);
    }

    pthread_mutex_unlock(&logging_config.log_lock);
}

/*
 * tecc_key_log_hexdump
 *    Log hex dump of data
 */
void
tecc_key_log_hexdump(LogLevel level, const char *label,
                    const void *data, size_t length)
{
    const unsigned char *bytes = (const unsigned char *)data;
    char hex_line[128];
    char ascii_line[32];
    size_t i, j;
    int offset = 0;

    if (data == NULL || length == 0)
        return;

    tecc_key_log(level, __func__, __FILE__, __LINE__, "%s (%zu bytes):", label, length);

    for (i = 0; i < length; i += 16)
    {
        offset = 0;
        offset += snprintf(hex_line + offset, sizeof(hex_line) - offset, "  %04zx: ", i);

        /* Hex bytes */
        for (j = 0; j < 16; j++)
        {
            if (i + j < length)
                offset += snprintf(hex_line + offset, sizeof(hex_line) - offset, "%02x ", bytes[i + j]);
            else
                offset += snprintf(hex_line + offset, sizeof(hex_line) - offset, "   ");
        }

        offset += snprintf(hex_line + offset, sizeof(hex_line) - offset, " ");

        /* ASCII representation */
        for (j = 0; j < 16 && i + j < length; j++)
        {
            unsigned char c = bytes[i + j];
            ascii_line[j] = (c >= 32 && c <= 126) ? c : '.';
        }
        ascii_line[j] = '\0';

        snprintf(hex_line + offset, sizeof(hex_line) - offset, "|%s|", ascii_line);

        tecc_key_log(level, __func__, __FILE__, __LINE__, "%s", hex_line);
    }
}