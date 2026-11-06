/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_processor_internal.c
 *
 * Abstract:
 *   Internal utility functions for TECC processor module.
 *   Implements common utilities for memory management, string operations,
 *   error handling, and other shared functionality.
 *
 * Authored by <wangxp@highgo.com>, 20260403.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/internal/tecc_processor_internal.c
 */

#include "postgres_fe.h"
#include "tecc_processor_internal.h"
#include <stdarg.h>
#include <string.h>

/* Frontend memory management */
#include "common/fe_memutils.h"

/* Include frontend parser node definitions */
#include "../../frontend_parser/tecc_nodes.h"  /* For foreach macro */

/* Include raw values list definitions */
#include "../raw_values_list.h"

/* Include libpq integration for tecc_get_state and cache API */
#include "../../tecc_libpq_integration/fe-tecc-integration.h"

/* Begin - ReqID:TECC4.0 - 线程安全调试支持 */

/* 全局调试级别变量（向后兼容，已废弃） */
/* 注意：这个变量不是线程安全的，仅用于向后兼容 */
/* 新代码应该使用 TeccState->config.debug_level 作为连接特定的调试级别 */
int tecc_processor_debug_level = TECC_LOG_DEBUG1;

/* End - ReqID:TECC4.0 - 线程安全调试支持 */

/*
 * Memory management utilities
 * Note: Frontend code must use pg_malloc()/pg_free() from fe_memutils.h
 * The deprecated tecc_processor_palloc/pfree functions have been removed.
 */

/*
 * String utilities
 */
char *
tecc_processor_strdup(const char *string)
{
    char *result;
    size_t len;

    if (string == NULL)
        return NULL;

    len = strlen(string) + 1;
    result = pg_malloc(len);
    memcpy(result, string, len);
    return result;
}

bool
tecc_processor_string_equal(const char *str1, const char *str2)
{
    if (str1 == NULL && str2 == NULL)
        return true;
    if (str1 == NULL || str2 == NULL)
        return false;
    return strcmp(str1, str2) == 0;
}

size_t
tecc_processor_string_length(const char *string)
{
    if (string == NULL)
        return 0;
    return strlen(string);
}

/*
 * List utilities
 */
List *
tecc_processor_list_append_string(List *list, const char *string)
{
    char *copy;

    if (string == NULL)
        return list;

    copy = pg_strdup(string);
    return lappend(list, copy);
}

bool
tecc_processor_list_member_string(List *list, const char *string)
{
    ListCell *lc;

    if (list == NULL || string == NULL)
        return false;

    foreach(lc, list)
    {
        if (strcmp((char *) lfirst(lc), string) == 0)
            return true;
    }
    return false;
}

void
tecc_processor_list_free_deep(List *list)
{
    if (list == NULL)
        return;

    /* 使用前端解析器的list_free_deep函数，它会调用tecc_pfree -> pg_free */
    /* 注意：需要包含前端解析器的头文件 */
    list_free_deep(list);
}

/*
 * Crypto buffer management utilities
 */

/*
 * tecc_processor_create_buffer - Create buffer owned by scheduler
 *
 * Creates a buffer that will be owned and freed by the scheduler.
 * Used for long-lived buffers that pass through multiple modules.
 */
TeccCryptoBuffer *
tecc_processor_create_buffer(size_t size)
{
    TeccCryptoBuffer *buffer;

    if (size == 0)
        return NULL;

    /* Allocate buffer structure */
    buffer = pg_malloc(sizeof(TeccCryptoBuffer));
    if (!buffer)
        return NULL;

    /* Allocate data buffer */
    buffer->data = pg_malloc(size);
    if (!buffer->data) {
        pg_free(buffer);
        return NULL;
    }

    /* Initialize buffer */
    buffer->size = size;
    buffer->used = 0;
    buffer->owned_by_caller = false;  /* Scheduler owns this buffer */
    buffer->next = NULL;

    return buffer;
}

/*
 * tecc_processor_borrow_buffer - Borrow buffer for temporary use
 *
 * Creates a buffer that the caller is responsible for freeing.
 * Used for short-lived buffers within a single module.
 */
TeccCryptoBuffer *
tecc_processor_borrow_buffer(size_t size)
{
    TeccCryptoBuffer *buffer;

    if (size == 0)
        return NULL;

    /* Allocate buffer structure */
    buffer = pg_malloc(sizeof(TeccCryptoBuffer));
    if (!buffer)
        return NULL;

    /* Allocate data buffer */
    buffer->data = pg_malloc(size);
    if (!buffer->data) {
        pg_free(buffer);
        return NULL;
    }

    /* Initialize buffer */
    buffer->size = size;
    buffer->used = 0;
    buffer->owned_by_caller = true;  /* Caller owns this buffer */
    buffer->next = NULL;

    return buffer;
}

/*
 * tecc_processor_release_buffer - Release buffer
 *
 * Frees a buffer if the caller owns it (owned_by_caller is true).
 * For scheduler-owned buffers, this just resets the used counter.
 */
void
tecc_processor_release_buffer(TeccCryptoBuffer *buffer)
{
    if (!buffer)
        return;

    if (buffer->owned_by_caller) {
        /* Caller owns this buffer, free everything */
        if (buffer->data)
            pg_free(buffer->data);
        pg_free(buffer);
    } else {
        /* Scheduler owns this buffer, just reset used counter */
        buffer->used = 0;
    }
}

/*
 * tecc_processor_reset_buffer - Reset buffer for reuse
 *
 * Resets a buffer for reuse, optionally resizing it.
 * For scheduler-owned buffers, this can resize the buffer.
 * For caller-owned buffers, this just resets the used counter.
 */
void
tecc_processor_reset_buffer(TeccCryptoBuffer *buffer, size_t new_size)
{
    if (!buffer)
        return;

    if (new_size > buffer->size) {
        /* Need to resize buffer */
        if (buffer->owned_by_caller) {
            /* Caller-owned buffer, can't resize - create new one */
            /* Note: In practice, caller should create a new buffer of appropriate size */
            buffer->used = 0;
            return;
        } else {
            /* Scheduler-owned buffer, can resize */
            void *new_data = pg_malloc(new_size);
            if (new_data) {
                if (buffer->data)
                    pg_free(buffer->data);
                buffer->data = new_data;
                buffer->size = new_size;
                buffer->used = 0;
            }
            /* If allocation fails, keep old buffer but reset used */
            buffer->used = 0;
        }
    } else {
        /* No resize needed, just reset used counter */
        buffer->used = 0;
    }
}

/*
 * Error handling utilities
 */
void
tecc_processor_set_error(TeccStatementData *stmt_data,
                        int error_code,
                        const char *error_format, ...)
{
    va_list args;

    if (stmt_data == NULL)
        return;

    stmt_data->error_code = error_code;

    if (error_format != NULL)
    {
        va_start(args, error_format);
        vsnprintf(stmt_data->error_message, sizeof(stmt_data->error_message),
                 error_format, args);
        va_end(args);
    }
    else
    {
        stmt_data->error_message[0] = '\0';
    }
}

void
tecc_processor_clear_error(TeccStatementData *stmt_data)
{
    if (stmt_data == NULL)
        return;

    stmt_data->error_code = TECC_PROCESSOR_SUCCESS;
    stmt_data->error_message[0] = '\0';
}

/*
 * Get error message for error code
 */
const char *
tecc_processor_get_error_message(int error_code)
{
    switch (error_code)
    {
        case TECC_PROCESSOR_SUCCESS:
            return "Success";

        /* Syntax layer errors */
        case TECC_PROCESSOR_SYNTAX_ERROR:
            return "Syntax error in SQL statement";
        case TECC_PROCESSOR_PARSE_ERROR:
            return "Failed to parse SQL statement";
        case TECC_PROCESSOR_TOKENIZATION_ERROR:
            return "Failed to tokenize SQL statement";
        case TECC_PROCESSOR_SQL_SYNTAX_ERROR:
            return "Invalid SQL syntax";

        /* Semantic layer errors */
        case TECC_PROCESSOR_SEMANTIC_ERROR:
            return "Semantic error in SQL statement";
        case TECC_PROCESSOR_TYPE_MISMATCH:
            return "Data type mismatch";
        case TECC_PROCESSOR_INVALID_INPUT:
            return "Invalid input parameter";
        case TECC_PROCESSOR_UNSUPPORTED_STATEMENT:
            return "Unsupported SQL statement type";
        case TECC_PROCESSOR_UNSUPPORTED_OPERATION:
            return "Unsupported operation for encrypted column";
        case TECC_PROCESSOR_INVALID_COLUMN_REFERENCE:
            return "Invalid column reference";
        case TECC_PROCESSOR_INVALID_TABLE_REFERENCE:
            return "Invalid table reference";
        case TECC_PROCESSOR_INVALID_FUNCTION_CALL:
            return "Invalid function call";

        /* Data layer errors */
        case TECC_PROCESSOR_DATA_ERROR:
            return "Data processing error";
        case TECC_PROCESSOR_ENCRYPTION_ERROR:
            return "Encryption failed";
        case TECC_PROCESSOR_DECRYPTION_ERROR:
            return "Decryption failed";
        case TECC_PROCESSOR_KEY_ERROR:
            return "Key error (missing or invalid key)";
        case TECC_PROCESSOR_ALGORITHM_ERROR:
            return "Encryption algorithm error";
        case TECC_PROCESSOR_DATA_FORMAT_ERROR:
            return "Invalid data format";
        case TECC_PROCESSOR_DATA_SIZE_ERROR:
            return "Data size exceeds limit";

        /* Metadata and cache errors */
        case TECC_PROCESSOR_METADATA_ERROR:
            return "Metadata error";
        case TECC_PROCESSOR_CACHE_ERROR:
            return "Cache error";
        case TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS:
            return "No encrypted columns found";
        case TECC_PROCESSOR_MISSING_METADATA:
            return "Missing required metadata";
        case TECC_PROCESSOR_INVALID_METADATA:
            return "Invalid metadata";

        /* System and resource errors */
        case TECC_PROCESSOR_MEMORY_ERROR:
            return "Memory allocation failed";
        case TECC_PROCESSOR_RESOURCE_ERROR:
            return "Resource allocation failed";
        case TECC_PROCESSOR_IO_ERROR:
            return "I/O error";
        case TECC_PROCESSOR_NETWORK_ERROR:
            return "Network error";

        /* Internal errors */
        case TECC_PROCESSOR_INTERNAL_ERROR:
            return "Internal error";
        case TECC_PROCESSOR_LOGIC_ERROR:
            return "Logic error";
        case TECC_PROCESSOR_ASSERTION_FAILED:
            return "Assertion failed";

        /* Special processing states */
        case TECC_PROCESSOR_NEEDS_REWRITE:
            return "SQL statement needs rewrite";
        case TECC_PROCESSOR_PARTIAL_SUCCESS:
            return "Partial success (some operations failed)";
        case TECC_PROCESSOR_WARNING:
            return "Warning (non-fatal issue)";

        default:
            return "Unknown error";
    }
}

/*
 * Check if error is recoverable
 */
bool
tecc_processor_is_recoverable_error(int error_code)
{
    switch (error_code)
    {
        /* Recoverable errors */
        case TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS:
        case TECC_PROCESSOR_NEEDS_REWRITE:
        case TECC_PROCESSOR_PARTIAL_SUCCESS:
        case TECC_PROCESSOR_WARNING:
        case TECC_PROCESSOR_CACHE_ERROR:          /* Can retry cache load */
        case TECC_PROCESSOR_METADATA_ERROR:       /* Can retry metadata load */
        case TECC_PROCESSOR_MISSING_METADATA:     /* Can load missing metadata */
        case TECC_PROCESSOR_RESOURCE_ERROR:       /* Can retry resource allocation */
        case TECC_PROCESSOR_IO_ERROR:             /* Can retry I/O operation */
        case TECC_PROCESSOR_NETWORK_ERROR:        /* Can retry network operation */
            return true;

        /* Non-recoverable errors */
        case TECC_PROCESSOR_SYNTAX_ERROR:
        case TECC_PROCESSOR_PARSE_ERROR:
        case TECC_PROCESSOR_TOKENIZATION_ERROR:
        case TECC_PROCESSOR_SQL_SYNTAX_ERROR:
        case TECC_PROCESSOR_SEMANTIC_ERROR:
        case TECC_PROCESSOR_TYPE_MISMATCH:
        case TECC_PROCESSOR_INVALID_INPUT:
        case TECC_PROCESSOR_UNSUPPORTED_STATEMENT:
        case TECC_PROCESSOR_UNSUPPORTED_OPERATION:
        case TECC_PROCESSOR_INVALID_COLUMN_REFERENCE:
        case TECC_PROCESSOR_INVALID_TABLE_REFERENCE:
        case TECC_PROCESSOR_INVALID_FUNCTION_CALL:
        case TECC_PROCESSOR_ENCRYPTION_ERROR:
        case TECC_PROCESSOR_DECRYPTION_ERROR:
        case TECC_PROCESSOR_KEY_ERROR:
        case TECC_PROCESSOR_ALGORITHM_ERROR:
        case TECC_PROCESSOR_DATA_FORMAT_ERROR:
        case TECC_PROCESSOR_DATA_SIZE_ERROR:
        case TECC_PROCESSOR_INVALID_METADATA:
        case TECC_PROCESSOR_MEMORY_ERROR:
        case TECC_PROCESSOR_INTERNAL_ERROR:
        case TECC_PROCESSOR_LOGIC_ERROR:
        case TECC_PROCESSOR_ASSERTION_FAILED:
        default:
            return false;
    }
}

/*
 * Attempt to recover from error
 * Returns true if recovery was successful, false otherwise
 */
bool
tecc_processor_attempt_recovery(TeccStatementData *stmt_data, int error_code)
{
    if (stmt_data == NULL)
        return false;

    /* Check if error is recoverable */
    if (!tecc_processor_is_recoverable_error(error_code))
        return false;

    

    /* Implement recovery strategies based on error type */
    switch (error_code)
    {
        case TECC_PROCESSOR_CACHE_ERROR:
        case TECC_PROCESSOR_METADATA_ERROR:
        case TECC_PROCESSOR_MISSING_METADATA:
            /* Try to reload metadata cache */
            tecc_processor_debug(TECC_LOG_DEBUG2,
                                "Retrying metadata cache load");
            /* ReqID:TECC4.0 - Clear cache to force reload on next access */
            {
                TeccState *state = tecc_get_state(stmt_data->conn);
                if (state != NULL)
                    tecc_cache_clear(state);
                tecc_processor_debug(TECC_LOG_DEBUG2,
                                    "Cache cleared, will reload on next access");
            }
            break;

        case TECC_PROCESSOR_RESOURCE_ERROR:
            /* Try to free resources and retry */
            tecc_processor_debug(TECC_LOG_DEBUG2,
                                "Retrying resource allocation");
            /* ReqID:TECC4.0 - Implement resource cleanup and retry */
            /* Free any allocated resources and retry the operation */
            if (stmt_data != NULL && stmt_data->raw_values_list != NULL)
            {
                tecc_raw_values_list_destroy(stmt_data->raw_values_list);
                stmt_data->raw_values_list = NULL;
                tecc_processor_debug(TECC_LOG_DEBUG2,
                                    "Freed raw values list, retrying");
            }
            break;

        case TECC_PROCESSOR_IO_ERROR:
        case TECC_PROCESSOR_NETWORK_ERROR:
            /* Log error and continue with degraded functionality */
            tecc_processor_debug(TECC_LOG_WARNING,
                                "Continuing with degraded functionality after I/O error");
            /* Clear error to allow continuation */
            tecc_processor_clear_error(stmt_data);
            return true;

        case TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS:
            /* This is not actually an error - just a state */
            tecc_processor_clear_error(stmt_data);
            return true;

        case TECC_PROCESSOR_NEEDS_REWRITE:
            /* This is expected behavior for statements with encrypted columns */
            tecc_processor_clear_error(stmt_data);
            return true;

        default:
            /* No specific recovery strategy for this error */
            break;
    }

    /* If we reach here, recovery was not successful */
    return false;
}

/*
 * Set error with automatic recovery attempt
 * Returns true if error was recovered, false otherwise
 */
bool
tecc_processor_set_error_with_recovery(TeccStatementData *stmt_data,
                                      int error_code,
                                      const char *error_format, ...)
{
    va_list args;
    bool recovered = false;

    if (stmt_data == NULL)
        return false;

    /* First set the error */
    stmt_data->error_code = error_code;

    if (error_format != NULL)
    {
        va_start(args, error_format);
        vsnprintf(stmt_data->error_message, sizeof(stmt_data->error_message),
                 error_format, args);
        va_end(args);
    }
    else
    {
        stmt_data->error_message[0] = '\0';
    }

    /* Log the error */
    tecc_processor_debug(TECC_LOG_ERROR,
                        "Error set: %s - %s",
                        tecc_processor_get_error_message(error_code),
                        stmt_data->error_message);

    /* Attempt recovery if error is recoverable */
    if (tecc_processor_is_recoverable_error(error_code))
    {
        recovered = tecc_processor_attempt_recovery(stmt_data, error_code);
        if (recovered)
        {
            
        }
        else
        {
            tecc_processor_debug(TECC_LOG_WARNING,
                                "Error recovery failed");
        }
    }

    return recovered;
}
/* End - ReqID:TECC4.0 */
