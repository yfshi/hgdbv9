/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_copy_state.c
 *
 * Abstract:
 *   COPY state management functions implementation for TECC import/export module.
 *   Provides functions for managing TeccEnhancedCopyState structure.
 *
 * Authored by <wangxp@highgo.com>, 20260417.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_common/tecc_copy_state.c
 */

#include "postgres_fe.h"
#include "tecc_common_types.h"
#include "../tecc_cache/tecc_metadata_cache.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>  /* For gettimeofday */

/* ==================== 错误处理函数 ==================== */

/*
 * tecc_copy_state_add_error - Add error information to COPY state
 *
 * Parameters:
 *   copy_state - Enhanced COPY state structure
 *   error_code - Error code
 *   error_msg - Error message
 *   row_number - Row number where error occurred
 *   column_index - Column index where error occurred (0-based)
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int
tecc_copy_state_add_error(TeccEnhancedCopyState *copy_state,
                         TeccCopyErrorCode error_code,
                         const char *error_msg,
                         int row_number,
                         int column_index)
{
    TeccCopyError *error;

    /* Validate parameters */
    if (copy_state == NULL || error_msg == NULL) {
        return -1;
    }

    /* Check if we've reached maximum error count */
    if (copy_state->error_count >= copy_state->max_errors && copy_state->max_errors > 0) {
        /* Too many errors, cannot add more */
        return -1;
    }

    /* Allocate or reallocate errors array if needed */
    if (copy_state->errors == NULL) {
        /* Initial allocation */
        copy_state->errors = (TeccCopyError *)pg_malloc(sizeof(TeccCopyError) * copy_state->max_errors);
        if (copy_state->errors == NULL) {
            return -1;
        }
        copy_state->error_count = 0;
    } else if (copy_state->error_count >= copy_state->max_errors) {
        /* Need to reallocate */
        int new_max_errors = copy_state->max_errors * 2;
        TeccCopyError *new_errors = (TeccCopyError *)pg_realloc(copy_state->errors,
                                                                sizeof(TeccCopyError) * new_max_errors);
        if (new_errors == NULL) {
            return -1;
        }
        copy_state->errors = new_errors;
        copy_state->max_errors = new_max_errors;
    }

    /* Add error to array */
    error = &copy_state->errors[copy_state->error_count];
    error->error_code = error_code;

    /* Safely copy error message with bounds checking */
    if (error_msg != NULL) {
        strncpy(error->error_msg, error_msg, sizeof(error->error_msg) - 1);
        error->error_msg[sizeof(error->error_msg) - 1] = '\0';
    } else {
        error->error_msg[0] = '\0'; /* Empty string */
    }

    error->row_number = row_number;
    error->column_index = column_index;
    error->timestamp = time(NULL);

    copy_state->error_count++;

    /* Check if we should stop on error */
    if (copy_state->stop_on_error) {
        /* Mark COPY state as cancelled */
        copy_state->cancelled = true;
    }

    return 0;
}

/*
 * tecc_copy_state_update_progress - Update progress information
 *
 * Parameters:
 *   copy_state - Enhanced COPY state structure
 *   processed_bytes - Number of bytes processed
 *   processed_rows - Number of rows processed
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int
tecc_copy_state_update_progress(TeccEnhancedCopyState *copy_state,
                               uint64_t processed_bytes,
                               uint64_t processed_rows)
{
    struct timeval current_time;
    double elapsed_seconds;

    /* Validate parameters */
    if (copy_state == NULL) {
        return -1;
    }

    /* Update processed counts */
    copy_state->progress.processed_bytes = processed_bytes;
    copy_state->progress.processed_rows = processed_rows;

    /* Calculate bytes per second if enough time has passed */
    gettimeofday(&current_time, NULL);

    elapsed_seconds = (current_time.tv_sec - copy_state->progress.start_time.tv_sec) +
                     (current_time.tv_usec - copy_state->progress.start_time.tv_usec) / 1000000.0;

    if (elapsed_seconds > 0.1) { /* At least 0.1 seconds to avoid division by zero */
        copy_state->progress.bytes_per_second = processed_bytes / elapsed_seconds;
        copy_state->progress.rows_per_second = processed_rows / elapsed_seconds;
    }

    /* Update last update time */
    copy_state->progress.last_update = current_time;

    /* Update peak memory usage */
    if (copy_state->memory_used > copy_state->progress.peak_memory) {
        copy_state->progress.peak_memory = copy_state->memory_used;
    }

    return 0;
}

/*
 * tecc_copy_state_start_progress - Start progress tracking
 *
 * Parameters:
 *   copy_state - Enhanced COPY state structure
 *   total_bytes - Total bytes to process (0 if unknown)
 *   total_rows - Total rows to process (0 if unknown)
 *   show_progress - Whether to show progress bar
 *   interactive - Whether running in interactive terminal
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int
tecc_copy_state_start_progress(TeccEnhancedCopyState *copy_state,
                              uint64_t total_bytes,
                              uint64_t total_rows,
                              bool show_progress,
                              bool interactive)
{
    /* Validate parameters */
    if (copy_state == NULL) {
        return -1;
    }

    /* Initialize progress information */
    gettimeofday(&copy_state->progress.start_time, NULL);
    copy_state->progress.last_update = copy_state->progress.start_time;

    copy_state->progress.total_bytes = total_bytes;
    copy_state->progress.total_rows = total_rows;
    copy_state->progress.processed_bytes = 0;
    copy_state->progress.processed_rows = 0;

    copy_state->progress.bytes_per_second = 0.0;
    copy_state->progress.rows_per_second = 0.0;
    copy_state->progress.peak_memory = 0;

    copy_state->progress.show_progress = show_progress;
    copy_state->progress.interactive = interactive;

    /* Get terminal width for progress bar */
    copy_state->progress.terminal_width = 80; /* Default width */

    return 0;
}

/*
 * tecc_copy_state_end_progress - End progress tracking
 *
 * Parameters:
 *   copy_state - Enhanced COPY state structure
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int
tecc_copy_state_end_progress(TeccEnhancedCopyState *copy_state)
{
    /* Validate parameters */
    if (copy_state == NULL) {
        return -1;
    }

    /* Final progress update */
    tecc_copy_state_update_progress(copy_state,
                                   copy_state->progress.processed_bytes,
                                   copy_state->progress.processed_rows);

    return 0;
}

/* ==================== 内存管理函数 ==================== */

/*
 * tecc_copy_state_alloc_row_buffer - Allocate row buffer
 *
 * Parameters:
 *   copy_state - Enhanced COPY state structure
 *   required_size - Required buffer size
 *
 * Returns:
 *   Pointer to allocated buffer, NULL on failure
 */
char *
tecc_copy_state_alloc_row_buffer(TeccEnhancedCopyState *copy_state,
                                size_t required_size)
{
    size_t old_capacity;
    size_t new_memory_used;
    char *new_buffer;

    /* Validate parameters */
    if (copy_state == NULL || required_size == 0) {
        return NULL;
    }

    /* Check memory limit */
    if (copy_state->memory_limit > 0 &&
        copy_state->memory_used + required_size > copy_state->memory_limit) {
        /* Memory limit exceeded */
        return NULL;
    }

    /* Allocate or reallocate buffer */
    if (copy_state->row_buffer == NULL || copy_state->buffer_capacity < required_size) {
        /* Calculate new capacity (with some extra space) */
        size_t new_capacity = required_size * 2;
        if (new_capacity < 1024) {
            new_capacity = 1024; /* Minimum buffer size */
        }

        /* Check if new capacity exceeds memory limit */
        if (copy_state->memory_limit > 0 &&
            copy_state->memory_used - copy_state->buffer_capacity + new_capacity > copy_state->memory_limit) {
            /* Try to allocate exact size */
            new_capacity = required_size;

            if (copy_state->memory_limit > 0 &&
                copy_state->memory_used - copy_state->buffer_capacity + new_capacity > copy_state->memory_limit) {
                /* Still exceeds limit */
                return NULL;
            }
        }

        /* Update memory usage */
        old_capacity = copy_state->buffer_capacity;

        /* Calculate new memory usage */
        new_memory_used = copy_state->memory_used;
        if (copy_state->row_buffer == NULL) {
            /* First allocation: add new capacity */
            new_memory_used = copy_state->memory_used + new_capacity;
        } else {
            /* Reallocation: subtract old capacity, add new capacity */
            new_memory_used = copy_state->memory_used - old_capacity + new_capacity;
        }

        copy_state->memory_used = new_memory_used;
        copy_state->buffer_capacity = new_capacity;

        /* Allocate new buffer */
        new_buffer = (char *)pg_realloc(copy_state->row_buffer, new_capacity);
        if (new_buffer == NULL) {
            /* Restore original memory usage */
            if (copy_state->row_buffer == NULL) {
                /* First allocation failed: no change to memory_used */
                copy_state->memory_used = copy_state->memory_used - new_capacity;
            } else {
                /* Reallocation failed: restore old capacity */
                copy_state->memory_used = copy_state->memory_used + old_capacity - new_capacity;
            }
            copy_state->buffer_capacity = old_capacity;
            return NULL;
        }

        copy_state->row_buffer = new_buffer;
    }

    /* Update buffer size */
    copy_state->buffer_size = required_size;

    return copy_state->row_buffer;
}

/*
 * tecc_copy_state_free_row_buffer - Free row buffer
 *
 * Parameters:
 *   copy_state - Enhanced COPY state structure
 */
void
tecc_copy_state_free_row_buffer(TeccEnhancedCopyState *copy_state)
{
    /* Validate parameters */
    if (copy_state == NULL) {
        return;
    }

    /* Free buffer if allocated */
    if (copy_state->row_buffer != NULL) {
        /* Update memory usage */
        copy_state->memory_used -= copy_state->buffer_capacity;

        pg_free(copy_state->row_buffer);
        copy_state->row_buffer = NULL;
        copy_state->buffer_size = 0;
        copy_state->buffer_capacity = 0;
    }
}

/* ==================== 错误恢复函数 ==================== */

/*
 * tecc_copy_state_is_recoverable_error - Check if error is recoverable
 *
 * Parameters:
 *   error_code - Error code to check
 *
 * Returns:
 *   true if error is recoverable, false otherwise
 */
bool
tecc_copy_state_is_recoverable_error(TeccCopyErrorCode error_code)
{
    /* Define which error codes are recoverable */
    switch (error_code) {
        case TECC_COPY_OUT_OF_MEMORY:
            /* Memory errors might be recoverable if we free some memory */
            return true;
        case TECC_COPY_DB_CONN_ERROR:
            /* Connection errors might be recoverable with retry */
            return true;
        case TECC_COPY_DB_QUERY_ERROR:
            /* Query errors might be transient */
            return true;
        case TECC_COPY_TIMEOUT:
            /* Timeout errors are often recoverable */
            return true;
        case TECC_COPY_NETWORK_ERROR:
            /* Network errors are often recoverable */
            return true;
        case TECC_COPY_FILE_READ_ERROR:
        case TECC_COPY_FILE_WRITE_ERROR:
            /* File I/O errors might be recoverable */
            return true;
        case TECC_COPY_ENCRYPTION_ERROR:
        case TECC_COPY_DECRYPTION_ERROR:
        case TECC_COPY_KEY_INVALID:
            /* Crypto errors are usually not recoverable */
            return false;
        case TECC_COPY_DATA_CORRUPTED:
            /* Data corruption is not recoverable */
            return false;
        case TECC_COPY_INTERNAL_ERROR:
            /* Internal errors are usually not recoverable */
            return false;
        case TECC_COPY_SUCCESS:
            /* Success is not an error */
            return false;
        default:
            /* Unknown errors are not recoverable by default */
            return false;
    }
}

/*
 * tecc_copy_state_reset_errors - Reset error state in COPY state
 *
 * Parameters:
 *   copy_state - Enhanced COPY state structure
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int
tecc_copy_state_reset_errors(TeccEnhancedCopyState *copy_state)
{
    /* Validate parameters */
    if (copy_state == NULL) {
        return -1;
    }

    /* Reset error count */
    copy_state->error_count = 0;

    /* Free errors array if allocated */
    if (copy_state->errors != NULL) {
        pg_free(copy_state->errors);
        copy_state->errors = NULL;
    }

    /* Reset cancelled flag */
    copy_state->cancelled = false;

    return 0;
}

/*
 * tecc_copy_state_retry_operation - Retry operation with recovery
 *
 * Parameters:
 *   copy_state - Enhanced COPY state structure
 *   operation - Function pointer to operation to retry
 *   operation_arg - Argument to pass to operation
 *   max_retries - Maximum number of retries (default: 3)
 *   base_delay_ms - Base delay between retries in milliseconds (default: 100)
 *
 * Returns:
 *   0 on success, -1 on failure after all retries
 */
int
tecc_copy_state_retry_operation(TeccEnhancedCopyState *copy_state,
                                int (*operation)(TeccEnhancedCopyState *, void *),
                                void *operation_arg,
                                int max_retries,
                                int base_delay_ms)
{
    int retry_count = 0;
    int result;
    struct timeval start_time, current_time;
    int delay_ms;
    int jitter;
    double elapsed_seconds;
#ifndef _WIN32
    struct timespec sleep_time;
#endif

    /* Validate parameters */
    if (copy_state == NULL || operation == NULL) {
        return -1;
    }

    /* Set default values if not specified */
    if (max_retries <= 0) {
        max_retries = 3;
    }
    if (base_delay_ms <= 0) {
        base_delay_ms = 100;
    }

    gettimeofday(&start_time, NULL);

    /* Try operation with retries */
    while (retry_count <= max_retries) {
        result = operation(copy_state, operation_arg);

        if (result == 0) {
            /* Operation succeeded */
            return 0;
        }

        /* Check if error is recoverable */
        if (copy_state->error_count > 0) {
            TeccCopyErrorCode last_error = copy_state->errors[copy_state->error_count - 1].error_code;
            if (!tecc_copy_state_is_recoverable_error(last_error)) {
                /* Non-recoverable error, don't retry */
                return -1;
            }
        }

        retry_count++;

        if (retry_count > max_retries) {
            /* Max retries exceeded */
            return -1;
        }

        /* Calculate exponential backoff delay */
        delay_ms = base_delay_ms * (1 << (retry_count - 1)); /* Exponential backoff */
        if (delay_ms > 5000) { /* Cap at 5 seconds */
            delay_ms = 5000;
        }

        /* Add jitter (10% of delay) */
        jitter = (rand() % (delay_ms / 10)) - (delay_ms / 20);
        delay_ms += jitter;

        /* Check timeout (max 30 seconds total) */
        gettimeofday(&current_time, NULL);
        elapsed_seconds = (current_time.tv_sec - start_time.tv_sec) +
                         (current_time.tv_usec - start_time.tv_usec) / 1000000.0;

        if (elapsed_seconds > 30.0) {
            /* Timeout exceeded */
            return -1;
        }

        /* Sleep before retry */
#ifdef _WIN32
        Sleep(delay_ms);
#else
        sleep_time.tv_sec = delay_ms / 1000;
        sleep_time.tv_nsec = (delay_ms % 1000) * 1000000;
        nanosleep(&sleep_time, NULL);
#endif

        /* Reset error state before retry */
        tecc_copy_state_reset_errors(copy_state);
    }

    return -1;
}

/* ==================== COPY状态创建和销毁函数 ==================== */

/*
 * tecc_copy_state_init - Initialize COPY state
 *
 * Parameters:
 *   copy_state - COPY state to initialize
 *   is_from_stdin - Whether reading from stdin
 *   is_to_stdout - Whether writing to stdout
 *   csv_mode - Whether in CSV mode
 *   delimiter - Field delimiter character
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int
tecc_copy_state_init(TeccEnhancedCopyState *copy_state,
                    bool is_from_stdin,
                    bool is_to_stdout,
                    bool csv_mode,
                    char delimiter)
{
    if (copy_state == NULL) {
        return -1;
    }

    /* Set basic options */
    copy_state->is_from_stdin = is_from_stdin;
    copy_state->is_to_stdout = is_to_stdout;
    copy_state->csv_mode = csv_mode;
    copy_state->delimiter = delimiter;

    /* Set default CSV options if in CSV mode */
    if (csv_mode) {
        copy_state->quote = '"';
        copy_state->escape = '"';
        copy_state->null_string = "\\N";
    } else {
        copy_state->quote = '\0';
        copy_state->escape = '\0';
        copy_state->null_string = "\\N";
    }

    return 0;
}

/*
 * tecc_copy_state_set_format_options - Set format options
 *
 * Parameters:
 *   copy_state - COPY state
 *   quote - Quote character
 *   escape - Escape character
 *   header - Whether file has header
 *   null_string - NULL string representation
 *   binary_mode - Whether in binary mode
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int
tecc_copy_state_set_format_options(TeccEnhancedCopyState *copy_state,
                                  char quote,
                                  char escape,
                                  bool header,
                                  const char *null_string,
                                  bool binary_mode)
{
    if (copy_state == NULL) {
        return -1;
    }

    copy_state->quote = quote;
    copy_state->escape = escape;
    copy_state->header = header;
    copy_state->binary_mode = binary_mode;

    /* Copy null string if provided */
    if (null_string != NULL) {
        /* Free existing null string */
        if (copy_state->null_string != NULL) {
            pg_free(copy_state->null_string);
        }
        copy_state->null_string = pg_strdup(null_string);
        if (copy_state->null_string == NULL) {
            return -1;
        }
    }

    return 0;
}

/*
 * tecc_copy_state_set_performance_options - Set performance options
 *
 * Parameters:
 *   copy_state - COPY state
 *   batch_processing - Enable batch processing
 *   batch_size - Batch size
 *   stream_optimized - Enable stream optimization
 *   memory_limit - Memory limit in bytes (0 for unlimited)
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int
tecc_copy_state_set_performance_options(TeccEnhancedCopyState *copy_state,
                                       bool batch_processing,
                                       int batch_size,
                                       bool stream_optimized,
                                       size_t memory_limit)
{
    if (copy_state == NULL) {
        return -1;
    }

    copy_state->batch_processing = batch_processing;
    copy_state->batch_size = batch_size;
    copy_state->stream_optimized = stream_optimized;
    copy_state->memory_limit = memory_limit;

    /* Validate batch size */
    if (batch_size <= 0) {
        copy_state->batch_size = 1000; /* Default */
    }

    /* Set max batch size */
    copy_state->max_batch_size = batch_size * 10;
    if (copy_state->max_batch_size > 100000) {
        copy_state->max_batch_size = 100000; /* Cap at 100k */
    }

    return 0;
}

/*
 * tecc_copy_state_set_error_options - Set error handling options
 *
 * Parameters:
 *   copy_state - COPY state
 *   max_errors - Maximum number of errors before stopping
 *   stop_on_error - Whether to stop on first error
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int
tecc_copy_state_set_error_options(TeccEnhancedCopyState *copy_state,
                                 int max_errors,
                                 bool stop_on_error)
{
    if (copy_state == NULL) {
        return -1;
    }

    copy_state->max_errors = max_errors;
    copy_state->stop_on_error = stop_on_error;

    /* Validate max_errors */
    if (max_errors < 0) {
        copy_state->max_errors = 0; /* Unlimited */
    }

    return 0;
}


/*
 * tecc_copy_state_set_column_mapping - Set column mapping
 *
 * Parameters:
 *   copy_state - COPY state
 *   mapping - Column mapping array
 *   mapping_count - Number of mapping entries
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int
tecc_copy_state_set_column_mapping(TeccEnhancedCopyState *copy_state,
                                  const int *mapping,
                                  int mapping_count)
{
    if (copy_state == NULL || mapping == NULL || mapping_count <= 0) {
        return -1;
    }

    /* Free existing mapping if any */
    if (copy_state->column_mapping != NULL) {
        pg_free(copy_state->column_mapping);
    }

    /* Allocate new mapping array */
    copy_state->column_mapping = (int *)pg_malloc(sizeof(int) * mapping_count);
    if (copy_state->column_mapping == NULL) {
        return -1;
    }

    /* Copy mapping */
    memcpy(copy_state->column_mapping, mapping, sizeof(int) * mapping_count);
    copy_state->mapping_count = mapping_count;

    return 0;
}

/*
 * tecc_copy_state_create - Create enhanced COPY state structure
 *
 * Parameters:
 *   conn - Database connection
 *   table_name - Table name
 *   schema_name - Schema name (can be NULL)
 *   table_oid - Table OID (can be InvalidOid)
 *
 * Returns:
 *   Pointer to allocated COPY state, NULL on failure
 */
TeccEnhancedCopyState *
tecc_copy_state_create(PGconn *conn,
                      const char *table_name,
                      const char *schema_name,
                      Oid table_oid)
{
    TeccEnhancedCopyState *copy_state = NULL;

    /* Validate parameters */
    if (conn == NULL || table_name == NULL) {
        return NULL;
    }

    /* Allocate COPY state structure */
    copy_state = (TeccEnhancedCopyState *)pg_malloc(sizeof(TeccEnhancedCopyState));
    if (copy_state == NULL) {
        return NULL;
    }

    /* Zero-initialize the structure */
    memset(copy_state, 0, sizeof(TeccEnhancedCopyState));

    /* Copy table name */
    copy_state->table_name = pg_strdup(table_name);
    if (copy_state->table_name == NULL) {
        pg_free(copy_state);
        return NULL;
    }

    /* Copy schema name if provided */
    if (schema_name != NULL) {
        copy_state->schema_name = pg_strdup(schema_name);
        if (copy_state->schema_name == NULL) {
            pg_free(copy_state->table_name);
            pg_free(copy_state);
            return NULL;
        }
    }

    /* Set basic fields */
    copy_state->conn = conn;
    copy_state->table_oid = table_oid;

    /* Initialize default values */
    copy_state->delimiter = '\t';  /* Default tab delimiter */
    copy_state->quote = '"';       /* Default quote character */
    copy_state->escape = '"';      /* Default escape character */

    /* Allocate and initialize null string */
    copy_state->null_string = pg_strdup("\\N");
    if (copy_state->null_string == NULL) {
        if (copy_state->schema_name != NULL) {
            pg_free(copy_state->schema_name);
        }
        pg_free(copy_state->table_name);
        pg_free(copy_state);
        return NULL;
    }

    copy_state->batch_size = 1000; /* Default batch size */
    copy_state->max_batch_size = 10000; /* Default max batch size */

    copy_state->max_errors = 100;  /* Default max errors */
    copy_state->stop_on_error = true; /* Stop on error by default */

    copy_state->memory_limit = 0;  /* No memory limit by default */

    /* Initialize progress information */
    gettimeofday(&copy_state->progress.start_time, NULL);
    copy_state->progress.last_update = copy_state->progress.start_time;

    copy_state->initialized = true;

    return copy_state;
}

/*
 * tecc_copy_state_destroy - Destroy COPY state structure
 *
 * Parameters:
 *   copy_state - COPY state to destroy
 */
void
tecc_copy_state_destroy(TeccEnhancedCopyState *copy_state)
{
    if (copy_state == NULL) {
        return;
    }

    /* Free table name */
    if (copy_state->table_name != NULL) {
        pg_free(copy_state->table_name);
    }

    /* Free schema name */
    if (copy_state->schema_name != NULL) {
        pg_free(copy_state->schema_name);
    }

    /* Free null string */
    if (copy_state->null_string != NULL) {
        pg_free(copy_state->null_string);
    }

    /* Free column names array */
    if (copy_state->column_names != NULL) {
        for (int i = 0; i < copy_state->column_count; i++) {
            if (copy_state->column_names[i] != NULL) {
                pg_free(copy_state->column_names[i]);
            }
        }
        pg_free(copy_state->column_names);
    }

    /* Free encrypted columns array */
    if (copy_state->encrypted_column_info != NULL) {
        for (int i = 0; i < copy_state->encrypted_column_count; i++) {
            if (copy_state->encrypted_column_info[i] != NULL) {
                /* Free individual encrypted column metadata */
                if (copy_state->encrypted_column_info[i]->column_name != NULL) {
                    pg_free(copy_state->encrypted_column_info[i]->column_name);
                }
                /* Note: kek_name and dek_name might be pointers to cache, not owned by us */
                pg_free(copy_state->encrypted_column_info[i]);
            }
        }
        pg_free(copy_state->encrypted_column_info);
    }

    /* Free column mapping */
    if (copy_state->column_mapping != NULL) {
        pg_free(copy_state->column_mapping);
    }

    /* Free errors array */
    if (copy_state->errors != NULL) {
        pg_free(copy_state->errors);
    }

    /* Free row buffer */
    if (copy_state->row_buffer != NULL) {
        pg_free(copy_state->row_buffer);
    }

    /* Free processor context */
    if (copy_state->processor_ctx != NULL) {
        /* TODO: Add cleanup function if needed */
        pg_free(copy_state->processor_ctx);
    }

    /* Free crypto context */
    if (copy_state->crypto_ctx != NULL) {
        /* TODO: Add cleanup function if needed */
        pg_free(copy_state->crypto_ctx);
    }

    /* Free formatter context */
    if (copy_state->formatter_ctx != NULL) {
        /* TODO: Add cleanup function if needed */
        pg_free(copy_state->formatter_ctx);
    }

    /* Free memory pool */
    if (copy_state->memory_pool != NULL) {
        /* TODO: Add cleanup function if needed */
        pg_free(copy_state->memory_pool);
    }

    /* Zero out the structure before freeing */
    memset(copy_state, 0, sizeof(TeccEnhancedCopyState));

    /* Free the structure itself */
    pg_free(copy_state);
}

/*
 * tecc_copy_state_reset - Reset COPY state for reuse
 *
 * Parameters:
 *   copy_state - COPY state to reset
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int
tecc_copy_state_reset(TeccEnhancedCopyState *copy_state)
{
    if (copy_state == NULL) {
        return -1;
    }

    /* Reset error state */
    tecc_copy_state_reset_errors(copy_state);

    /* Reset progress information */
    gettimeofday(&copy_state->progress.start_time, NULL);
    copy_state->progress.last_update = copy_state->progress.start_time;

    copy_state->progress.total_bytes = 0;
    copy_state->progress.processed_bytes = 0;
    copy_state->progress.total_rows = 0;
    copy_state->progress.processed_rows = 0;
    copy_state->progress.bytes_per_second = 0.0;
    copy_state->progress.rows_per_second = 0.0;
    copy_state->progress.peak_memory = 0;

    /* Reset current row */
    copy_state->current_row = 0;

    /* Reset state flags */
    copy_state->processing = false;
    copy_state->completed = false;
    copy_state->cancelled = false;

    /* Free row buffer */
    tecc_copy_state_free_row_buffer(copy_state);

    return 0;
}

/*
 * tecc_copy_state_is_valid - Check if COPY state is valid
 *
 * Parameters:
 *   copy_state - COPY state to check
 *
 * Returns:
 *   true if valid, false otherwise
 */
bool
tecc_copy_state_is_valid(const TeccEnhancedCopyState *copy_state)
{
    if (copy_state == NULL) {
        return false;
    }

    /* Check basic required fields */
    if (copy_state->conn == NULL || copy_state->table_name == NULL) {
        return false;
    }

    /* Check if initialized */
    if (!copy_state->initialized) {
        return false;
    }

    /* Check if cancelled */
    if (copy_state->cancelled) {
        return false;
    }

    return true;
}

/*
 * tecc_copy_state_can_continue - Check if COPY processing can continue
 *
 * Parameters:
 *   copy_state - COPY state to check
 *
 * Returns:
 *   true if can continue, false otherwise
 */
bool
tecc_copy_state_can_continue(const TeccEnhancedCopyState *copy_state)
{
    if (!tecc_copy_state_is_valid(copy_state)) {
        return false;
    }

    /* Check if completed */
    if (copy_state->completed) {
        return false;
    }

    /* Check error count */
    if (copy_state->max_errors > 0 && copy_state->error_count >= copy_state->max_errors) {
        return false;
    }

    return true;
}

/*
 * tecc_copy_state_get_error_stats - Get error statistics
 *
 * Parameters:
 *   copy_state - COPY state
 *   total_errors - Output: total error count
 *   recoverable_errors - Output: recoverable error count
 *   fatal_errors - Output: fatal error count
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int
tecc_copy_state_get_error_stats(const TeccEnhancedCopyState *copy_state,
                               int *total_errors,
                               int *recoverable_errors,
                               int *fatal_errors)
{
    if (copy_state == NULL || total_errors == NULL ||
        recoverable_errors == NULL || fatal_errors == NULL) {
        return -1;
    }

    *total_errors = copy_state->error_count;
    *recoverable_errors = 0;
    *fatal_errors = 0;

    /* Count recoverable and fatal errors */
    for (int i = 0; i < copy_state->error_count; i++) {
        if (tecc_copy_state_is_recoverable_error(copy_state->errors[i].error_code)) {
            (*recoverable_errors)++;
        } else {
            (*fatal_errors)++;
        }
    }

    return 0;
}

/*
 * tecc_copy_state_get_performance_stats - Get performance statistics
 *
 * Parameters:
 *   copy_state - COPY state
 *   bytes_per_second - Output: bytes per second
 *   rows_per_second - Output: rows per second
 *   peak_memory - Output: peak memory usage
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int
tecc_copy_state_get_performance_stats(const TeccEnhancedCopyState *copy_state,
                                     double *bytes_per_second,
                                     double *rows_per_second,
                                     uint64_t *peak_memory)
{
    if (copy_state == NULL || bytes_per_second == NULL ||
        rows_per_second == NULL || peak_memory == NULL) {
        return -1;
    }

    *bytes_per_second = copy_state->progress.bytes_per_second;
    *rows_per_second = copy_state->progress.rows_per_second;
    *peak_memory = copy_state->progress.peak_memory;

    return 0;
}

/* End - ReqID:TECC4.0 */