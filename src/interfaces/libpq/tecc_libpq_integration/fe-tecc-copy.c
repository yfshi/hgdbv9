#ifdef _MSC_VER
#define TECC_ATTR_UNUSED
#else
#define TECC_ATTR_UNUSED __attribute__((unused))
#endif

/*-------------------------------------------------------------------------
 *
 * fe-tecc-copy.c
 *    TECC COPY data processing implementation
 *
 * Portions Copyright (c) 1996-2026, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/interfaces/libpq/fe-tecc-copy.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres_fe.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "common/fe_memutils.h"

#include "libpq-fe.h"
#include "libpq-int.h"
#include "fe-tecc-integration.h"
#include "fe-tecc-copy.h"

#include "../tecc_cache/tecc_metadata_cache.h"
#include "../tecc_processor/tecc_processor.h"
#include "../tecc_processor/internal/tecc_processor_internal.h"
#include "../tecc_processor/processor_utils.h"
#include "../tecc_scheduler/tecc_crypto_scheduler.h"
#include "../tecc_scheduler/tecc_key_manager_adapter.h"
#include "../tecc_common/tecc_frontend_list.h"

#include "../frontend_parser/tecc_parser.h"
#include "../frontend_parser/tecc_nodes.h"
#include "../frontend_parser/tecc_error.h"
#include "../../../common/tecc_types.h"

/* External declarations for utility functions from fe-tecc-integration.c */

/* Static function prototypes */
static TeccCopyState *tecc_create_copy_state(PGconn *conn, const char *command);
static void tecc_free_copy_state(TeccCopyState *copy_state);
static int tecc_parse_copy_with_frontend_parser(PGconn *conn, const char *command, char **table_name,
                                           char ***column_names, int *column_count,
                                           bool *is_from_stdin, bool *is_to_stdout,
                                           bool *csv_mode, char *delimiter,
                                           char *quote, char *escape,
                                           bool *header, char **null_string);
static int tecc_process_copy_row(TeccEnhancedCopyState *copy_state, const char *row_data,
                                 int row_len, char **processed_row, int *processed_len,
                                 bool is_import);
static int tecc_process_copy_row_stream(TeccEnhancedCopyState *copy_state, const char *row_data,
                                       int row_len, char **processed_row, int *processed_len,
                                       bool is_import);
static int tecc_copy_parse_csv_row(const char *row_data, int row_len,
                                  char delimiter, char quote, char escape,
                                  char ***fields, int **field_lens, int *field_count);
static int tecc_copy_parse_text_row(const char *row_data, int row_len,
                                   char delimiter, char ***fields, int **field_lens,
                                   int *field_count);
static int tecc_copy_format_csv_row(char **fields, int *field_lens, int field_count,
                                   char delimiter, char quote, char escape,
                                   char **row_data, int *row_len);
static int tecc_copy_format_text_row(char **fields, int *field_lens, int field_count,
                                    char delimiter, char **row_data, int *row_len);
static void tecc_copy_free_parsed_fields(char **fields, int *field_lens, int field_count);

/* COPY state helper functions (non-static, declared in tecc_common_types.h) */
int tecc_copy_state_load_encrypted_columns(TeccEnhancedCopyState *copy_state);
int tecc_copy_state_init_column_mapping(TeccEnhancedCopyState *copy_state);
int tecc_copy_state_init_processor_context(TeccEnhancedCopyState *copy_state);
int tecc_copy_state_init_crypto_context(TeccEnhancedCopyState *copy_state);
int tecc_copy_state_init_stream_contexts(TeccEnhancedCopyState *copy_state);
int tecc_copy_state_cleanup_stream_contexts(TeccEnhancedCopyState *copy_state);
/* ==================== COPY Processing Functions ==================== */

/*
 * tecc_preprocess_copy - Preprocess COPY command
 *
 * Returns: TeccCopyState pointer on success, NULL on failure
 */
TeccCopyState *
tecc_preprocess_copy(PGconn *conn, const char *command)
{
	TeccState *state;
	TeccCopyState *copy_state;

	if (conn == NULL || command == NULL)
	{
		return NULL;
	}

	state = tecc_get_state(conn);
	if (state == NULL || !state->enabled)
	{
		/* TECC not enabled for this connection */
		return NULL;
	}

	/* Create and initialize COPY state.
	 * Only client-side COPY (FROM STDIN / TO STDOUT, used by psql \copy)
	 * needs TECC preprocessing. Server-side file COPY (TO/FROM 'filename')
	 * is not handled by the frontend parser's grammar, so parse failure here
	 * is expected for those cases and should NOT block the query. */
	copy_state = tecc_create_copy_state(conn, command);
	if (copy_state == NULL)
	{
		tecc_processor_debug(TECC_LOG_DEBUG1,
			"tecc_preprocess_copy: COPY state creation skipped (server-side file COPY or parse error)");
		/*
		 * Don't cleanup existing state on failure — it may have been set
		 * by a prior call (e.g., copy.c:471 before SendQuery/PQexec).
		 * Keeping it allows PQputCopyData encryption to still work.
		 */
		return NULL;
	}

	/* Successfully created new state — replace old one */
	tecc_cleanup_copy_state(conn);
	conn->tecc_copy_state = copy_state;

	return copy_state;
}

/* Begin - ReqID:TECC4.0 - COPY data processing implementation */
/*
 * tecc_process_copy_data - Process COPY data for encryption
 *
 * This function processes COPY data for encryption during import (COPY FROM STDIN).
 * It splits the data into rows, processes each row through the processor module,
 * and returns the encrypted data.
 *
 * Returns: 0 on success, -1 on failure
 */
int
tecc_process_copy_data(PGconn *conn, const char *data, int data_len,
                       char **processed_data, int *processed_len)
{
	TeccState *state;
	TeccEnhancedCopyState *copy_state;
	char *result_buffer = NULL;
	int result_buffer_size = 0;
	int result_buffer_pos = 0;
	const char *current_pos = data;
	const char *end_pos = data + data_len;
	int process_result = 0;  /* 处理结果变量 */

	/* Enhanced input validation */
	if (conn == NULL || data == NULL || processed_data == NULL || processed_len == NULL)
	{
		return -1;
	}

	/* Validate data length */
	if (data_len < 0)
	{
		/* Invalid data length */
		return -1;
	}

	/* Check for empty data */
	if (data_len == 0)
	{
		*processed_data = (char *)data;
		*processed_len = 0;
		return 0;
	}

	state = tecc_get_state(conn);
	if (state == NULL || !state->enabled)
	{
		/* TECC not enabled, return original data */
		*processed_data = (char *)data;
		*processed_len = data_len;
		return 0;
	}

	copy_state = (TeccEnhancedCopyState *)conn->tecc_copy_state;
	if (copy_state == NULL || !copy_state->is_from_stdin)
	{
		/* Not in COPY FROM STDIN mode, return original data */
		*processed_data = (char *)data;
		*processed_len = data_len;
		return 0;
	}

	/* Check if there are encrypted columns to process */
	if (copy_state->encrypted_column_count == 0)
	{
		/* No encrypted columns, return original data */
		*processed_data = (char *)data;
		*processed_len = data_len;
		return 0;
	}

	/* Check if batch processing is enabled */
	if (copy_state->batch_processing && copy_state->batch_size > 0)
	{
		int rows_processed, errors_found;
		int result;

		/* Use batch processing mode */
		result = tecc_process_copy_data_batch(conn, data, data_len,
		                                      processed_data, processed_len,
		                                      &rows_processed, &errors_found);

		/* Log batch processing result */
		if (result == 0)
		{
			tecc_record_debug(tecc_get_state(conn),
			                  "Batch processing completed: %d rows, %d errors",
			                  rows_processed, errors_found);
		}
		else
		{
			tecc_record_debug(tecc_get_state(conn),
			                  "Batch processing failed: %d rows, %d errors",
			                  rows_processed, errors_found);
		}

		return result;
	}

	/* Initialize result buffer */
	result_buffer_size = data_len * 2; /* Allocate extra space for potential encryption expansion */
	result_buffer = (char *)pg_malloc(result_buffer_size);
	if (result_buffer == NULL)
	{
		tecc_handle_error(conn, TECC_ERR_MEMORY, "Failed to allocate memory for COPY data processing");
		return -1;
	}

	/* Process data line by line */
	while (current_pos < end_pos)
	{
		const char *line_start = current_pos;
		const char *line_end = current_pos;
		int line_len = 0;

		/* Find end of line (newline or end of data) */
		while (line_end < end_pos && *line_end != '\n')
		{
			line_end++;
		}

		line_len = line_end - line_start;

		/* Skip header line if CSV HEADER is enabled (first row only) */
		if (line_len > 0 && copy_state->header && copy_state->current_row == 0)
		{
			/* Copy header line as-is to result buffer */
			if (result_buffer_pos + line_len + 1 > result_buffer_size)
			{
				int hdr_new_size = result_buffer_size * 2;
				char *hdr_new_buffer = (char *)pg_realloc(result_buffer, hdr_new_size);
				if (hdr_new_buffer == NULL)
				{
					tecc_pfree(result_buffer);
					tecc_handle_error(conn, TECC_ERR_MEMORY, "Failed to expand COPY data buffer");
					return -1;
				}
				result_buffer = hdr_new_buffer;
				result_buffer_size = hdr_new_size;
			}
			memcpy(result_buffer + result_buffer_pos, line_start, line_len);
			result_buffer_pos += line_len;
			copy_state->current_row++;
			current_pos = line_end;
			if (current_pos < end_pos && *current_pos == '\n')
			{
				result_buffer[result_buffer_pos++] = '\n';
				current_pos++;
			}
			continue;
		}

		/* Process the line if it's not empty */
		if (line_len > 0)
		{
			char *processed_row = NULL;
			int processed_row_len = 0;

			/* Process the row using appropriate function based on stream optimization */
			if (copy_state->stream_optimized && copy_state->stream_contexts_initialized) {
				/* Use stream-optimized processing */
				process_result = tecc_process_copy_row_stream(copy_state, line_start, line_len,
				                                              &processed_row, &processed_row_len, true);
			} else {
				/* Use standard processing */
				process_result = tecc_process_copy_row(copy_state, line_start, line_len,
				                                       &processed_row, &processed_row_len, true);
			}

			if (process_result != 0)
			{
				/* Error processing row */
				tecc_pfree(result_buffer);
				tecc_handle_error(conn, TECC_ERR_PROCESSOR, "Failed to process COPY data row");
				return -1;
			}

			/* Check if we need to expand result buffer */
			while (result_buffer_pos + processed_row_len + 1 > result_buffer_size)
			{
				int new_size = result_buffer_size * 2;
				char *new_buffer = (char *)pg_realloc(result_buffer, new_size);
				if (new_buffer == NULL)
				{
					tecc_pfree(result_buffer);
					tecc_handle_error(conn, TECC_ERR_MEMORY, "Failed to expand COPY data buffer");
					return -1;
				}
				result_buffer = new_buffer;
				result_buffer_size = new_size;
			}

			/* Copy processed row to result buffer */
			memcpy(result_buffer + result_buffer_pos, processed_row, processed_row_len);
			result_buffer_pos += processed_row_len;

			/* Free processed row memory */
			if (processed_row != (char *)line_start) /* Only free if it's a new allocation */
			{
				tecc_pfree(processed_row);
			}
		}

		/* Move to next line (skip newline character) */
		current_pos = line_end;
		if (current_pos < end_pos && *current_pos == '\n')
		{
			/* Add newline to result buffer */
			if (result_buffer_pos + 1 > result_buffer_size)
			{
				int new_size = result_buffer_size + 1;
				char *new_buffer = (char *)pg_realloc(result_buffer, new_size);
				if (new_buffer == NULL)
				{
					tecc_pfree(result_buffer);
					tecc_handle_error(conn, TECC_ERR_MEMORY, "Failed to expand COPY data buffer for newline");
					return -1;
				}
				result_buffer = new_buffer;
				result_buffer_size = new_size;
			}
			result_buffer[result_buffer_pos++] = '\n';
			current_pos++;
		}
	}

	/* Return processed data */
	*processed_data = result_buffer;
	*processed_len = result_buffer_pos;

	return 0;
}
/* End - ReqID:TECC4.0 */

/* Begin - ReqID:TECC4.0 - COPY result processing implementation */
/*
 * tecc_process_copy_result - Process COPY result for decryption
 *
 * This function processes COPY result for decryption during export (COPY TO STDOUT).
 * It splits the encrypted data into rows, processes each row through the processor module,
 * and returns the decrypted data.
 *
 * Returns: 0 on success, -1 on failure
 */
int
tecc_process_copy_result(PGconn *conn, const char *encrypted_data,
                         int encrypted_len, char **decrypted_data,
                         int *decrypted_len)
{
	TeccState *state;
	TeccEnhancedCopyState *copy_state;
	char *result_buffer = NULL;
	int result_buffer_size = 0;
	int result_buffer_pos = 0;
	const char *current_pos = encrypted_data;
	const char *end_pos = encrypted_data + encrypted_len;

	/* Enhanced input validation */
	if (conn == NULL || encrypted_data == NULL ||
		decrypted_data == NULL || decrypted_len == NULL)
	{
		tecc_handle_error(conn, TECC_ERR_PARAM, "Invalid parameters in tecc_process_copy_result");
		return -1;
	}

	/* Validate data length */
	if (encrypted_len < 0)
	{
		tecc_handle_error(conn, TECC_ERR_PARAM, "Invalid data length in tecc_process_copy_result");
		return -1;
	}

	/* Check for empty data */
	if (encrypted_len == 0)
	{
		/* Return empty result for empty input - consistent with tecc_process_copy_data */
		*decrypted_data = (char *)encrypted_data;
		*decrypted_len = 0;
		return 0;
	}

	state = tecc_get_state(conn);
	if (state == NULL || !state->enabled)
	{
		/* TECC not enabled, return original data */
		*decrypted_data = (char *)encrypted_data;
		*decrypted_len = encrypted_len;
		return 0;
	}

	copy_state = (TeccEnhancedCopyState *)conn->tecc_copy_state;
	if (copy_state == NULL || !copy_state->is_to_stdout)
	{
		/* Not in COPY TO STDOUT mode, return original data */
		*decrypted_data = (char *)encrypted_data;
		*decrypted_len = encrypted_len;
		return 0;
	}

	/* Check if there are encrypted columns to process */
	if (copy_state->encrypted_column_count == 0)
	{
		/* No encrypted columns, return original data */
		*decrypted_data = (char *)encrypted_data;
		*decrypted_len = encrypted_len;
		return 0;
	}

	/* Check if batch processing is enabled */
	if (copy_state->batch_processing && copy_state->batch_size > 0)
	{
		int rows_processed, errors_found;
		int result;

		/* Use batch processing mode */
		result = tecc_process_copy_result_batch(conn, encrypted_data, encrypted_len,
		                                        decrypted_data, decrypted_len,
		                                        &rows_processed, &errors_found);

		/* Log batch processing result */
		if (result == 0)
		{
			tecc_record_debug(tecc_get_state(conn),
			                  "Batch result processing completed: %d rows, %d errors",
			                  rows_processed, errors_found);
		}
		else
		{
			tecc_record_debug(tecc_get_state(conn),
			                  "Batch result processing failed: %d rows, %d errors",
			                  rows_processed, errors_found);
		}

		return result;
	}

	/* Initialize result buffer */
	result_buffer_size = encrypted_len * 2; /* Allocate extra space for potential decryption expansion */
	result_buffer = (char *)pg_malloc(result_buffer_size);
	if (result_buffer == NULL)
	{
		tecc_handle_error(conn, TECC_ERR_MEMORY, "Failed to allocate memory for COPY result processing");
		return -1;
	}

	/* Process data line by line */
	while (current_pos < end_pos)
	{
		const char *line_start = current_pos;
		const char *line_end = current_pos;
		int line_len = 0;

		/* Find end of line (newline or end of data) */
		while (line_end < end_pos && *line_end != '\n')
		{
			line_end++;
		}

		line_len = line_end - line_start;

		/* Process the line if it's not empty */
		if (line_len > 0)
		{
			char *decrypted_row = NULL;
			int decrypted_row_len = 0;
			int process_result;

			/* Process the row using appropriate function based on stream optimization */
			if (copy_state->stream_optimized && copy_state->stream_contexts_initialized) {
				/* Use stream-optimized processing */
				process_result = tecc_process_copy_row_stream(copy_state, line_start, line_len,
				                                              &decrypted_row, &decrypted_row_len, false);
			} else {
				/* Use standard processing */
				process_result = tecc_process_copy_row(copy_state, line_start, line_len,
				                                       &decrypted_row, &decrypted_row_len, false);
			}

			if (process_result != 0)
			{
				/* Error processing row */
				tecc_pfree(result_buffer);
				tecc_handle_error(conn, TECC_ERR_PROCESSOR, "Failed to process COPY result row");
				return -1;
			}

			/* Check if we need to expand result buffer */
			if (result_buffer_pos + decrypted_row_len + 1 > result_buffer_size)
			{
				int new_size = result_buffer_size * 2;
				char *new_buffer = (char *)pg_realloc(result_buffer, new_size);
				if (new_buffer == NULL)
				{
					tecc_pfree(result_buffer);
					tecc_handle_error(conn, TECC_ERR_MEMORY, "Failed to expand COPY result buffer");
					return -1;
				}
				result_buffer = new_buffer;
				result_buffer_size = new_size;
			}

			/* Copy decrypted row to result buffer */
			memcpy(result_buffer + result_buffer_pos, decrypted_row, decrypted_row_len);
			result_buffer_pos += decrypted_row_len;

			/* Free decrypted row memory */
			if (decrypted_row != (char *)line_start) /* Only free if it's a new allocation */
			{
				tecc_pfree(decrypted_row);
			}
		}

		/* Move to next line (skip newline character) */
		current_pos = line_end;
		if (current_pos < end_pos && *current_pos == '\n')
		{
			/* Add newline to result buffer */
			if (result_buffer_pos + 1 > result_buffer_size)
			{
				int new_size = result_buffer_size + 1;
				char *new_buffer = (char *)pg_realloc(result_buffer, new_size);
				if (new_buffer == NULL)
				{
					tecc_pfree(result_buffer);
					tecc_handle_error(conn, TECC_ERR_MEMORY, "Failed to expand COPY result buffer for newline");
					return -1;
				}
				result_buffer = new_buffer;
				result_buffer_size = new_size;
			}
			result_buffer[result_buffer_pos++] = '\n';
			current_pos++;
		}
	}

	/* Return processed data */
	*decrypted_data = result_buffer;
	*decrypted_len = result_buffer_pos;

	return 0;
}
/* End - ReqID:TECC4.0 */

/*
 * tecc_cleanup_copy_state - Clean up COPY state
 */
void
tecc_cleanup_copy_state(PGconn *conn)
{
	if (conn == NULL)
	{
		return;
	}

	if (conn->tecc_copy_state != NULL)
	{
		tecc_free_copy_state((TeccCopyState *)conn->tecc_copy_state);
		conn->tecc_copy_state = NULL;
	}
}

/*
 * tecc_create_copy_state - Create and initialize COPY state
 *
 * Returns: TeccCopyState pointer on success, NULL on failure
 */
static TeccCopyState *
tecc_create_copy_state(PGconn *conn, const char *command)
{
	TeccCopyState *copy_state;
	char *table_name = NULL;
	char **column_names = NULL;
	int column_count = 0;
	bool is_from_stdin = false;
	bool is_to_stdout = false;
	int i;

	/* Parse COPY command with format options */
	bool csv_mode = false;
	char delimiter = '\t';
	char quote = '"';
	char escape = '\\';
	bool header = false;
	char *null_string = NULL;

	if (tecc_parse_copy_with_frontend_parser(conn, command, &table_name, &column_names,
									   &column_count, &is_from_stdin, &is_to_stdout,
									   &csv_mode, &delimiter, &quote, &escape,
									   &header, &null_string) != 0)
	{
		return NULL;
	}

	/* COPY (SELECT ...) TO STDOUT — not a client-side COPY, no table name.
	 * Let it pass through to the backend unmodified. */
	if (table_name == NULL)
	{
		return NULL;
	}

	/* Allocate COPY state - using enhanced structure size */
	copy_state = (TeccCopyState *)pg_malloc(sizeof(TeccEnhancedCopyState));
	if (copy_state == NULL)
	{
		tecc_processor_debug(TECC_LOG_DEBUG1,
			"tecc_preprocess_copy: COPY state creation skipped (server-side file COPY or parse error)");
		/*
		 * Don't cleanup existing state on failure — it may have been set
		 * by a prior call (e.g., copy.c:471 before SendQuery/PQexec).
		 * Keeping it allows PQputCopyData encryption to still work.
		 */
		return NULL;
	}

	/* Initialize all fields to zero */
	memset(copy_state, 0, sizeof(TeccEnhancedCopyState));

	/* Set basic information */
	copy_state->table_name = table_name;
	copy_state->column_names = column_names;
	copy_state->column_count = column_count;
	copy_state->is_from_stdin = is_from_stdin;
	copy_state->is_to_stdout = is_to_stdout;
	copy_state->current_row = 0;

	/* Set default format values */
	copy_state->delimiter = delimiter;
	copy_state->quote = quote;
	copy_state->escape = escape;
	copy_state->csv_mode = csv_mode;
	copy_state->binary_mode = false; /* Binary mode not supported yet */
	copy_state->header = header;
	copy_state->null_string = null_string;

	/* Set default performance options */
	copy_state->batch_processing = false;  /* Disabled by default, enabled only when BATCH option is specified */
	copy_state->batch_size = 100;          /* Default batch size if enabled */
	copy_state->max_batch_size = 1000;     /* Default max batch size */
	copy_state->stream_optimized = true;

	/* Set default error handling options */
	copy_state->max_errors = 100;
	copy_state->stop_on_error = false;

	/* Initialize progress information */
	{
		struct timeval tv;
		gettimeofday(&tv, NULL);
		copy_state->progress.start_time = tv;
	}
	copy_state->progress.total_bytes = 0;
	copy_state->progress.total_rows = 0;
	copy_state->progress.processed_bytes = 0;
	copy_state->progress.processed_rows = 0;
	copy_state->progress.bytes_per_second = 0.0;
	copy_state->progress.rows_per_second = 0.0;
	copy_state->progress.peak_memory = 0;

	/* Set memory management defaults */
	copy_state->memory_used = 0;
	copy_state->memory_limit = 10 * 1024 * 1024; /* 10MB default limit */

	/* Set connection information */
	copy_state->conn = conn;

	/* Set state flags */
	copy_state->initialized = true;
	copy_state->processing = false;
	copy_state->completed = false;
	copy_state->cancelled = false;

	/* Load encrypted column information from cache */
	if (tecc_copy_state_load_encrypted_columns(copy_state) != 0)
	{
		/* Failed to load encrypted columns, but we can still continue with basic COPY */
		tecc_record_debug(NULL, "Failed to load encrypted column information, continuing with basic COPY");
		copy_state->encrypted_column_info = NULL;
		copy_state->encrypted_column_count = 0;
	}

	/* Initialize column mapping */
	if (tecc_copy_state_init_column_mapping(copy_state) != 0)
	{
		tecc_record_debug(NULL, "Failed to initialize column mapping");
		/* Continue anyway, mapping will be identity mapping */
	}

	/* Initialize processor context */
	if (tecc_copy_state_init_processor_context(copy_state) != 0)
	{
		tecc_record_debug(NULL, "Failed to initialize processor context");
		/* Continue without processor context */
	}

	/* Initialize crypto context */
	if (tecc_copy_state_init_crypto_context(copy_state) != 0)
	{
		tecc_record_debug(NULL, "Failed to initialize crypto context");
		/* Continue without crypto context */
	}

	return copy_state;

error_cleanup:
	if (table_name != NULL)
	{
		tecc_pfree(table_name);
	}
	if (column_names != NULL)
	{
		for (i = 0; i < column_count; i++)
		{
			if (column_names[i] != NULL)
			{
				tecc_pfree(column_names[i]);
			}
		}
		tecc_pfree(column_names);
	}
	if (null_string != NULL)
	{
		tecc_pfree(null_string);
	}
	if (copy_state != NULL)
	{
		tecc_pfree(copy_state);
	}
	return NULL;
}

/*
 * tecc_free_copy_state - Free COPY state
 */
static void
tecc_free_copy_state(TeccCopyState *copy_state)
{
	int i;

	if (copy_state == NULL)
	{
		tecc_processor_debug(TECC_LOG_DEBUG1,
			"tecc_preprocess_copy: COPY state creation skipped (server-side file COPY or parse error)");
		/*
		 * Don't cleanup existing state on failure — it may have been set
		 * by a prior call (e.g., copy.c:471 before SendQuery/PQexec).
		 * Keeping it allows PQputCopyData encryption to still work.
		 */
		return NULL;
	}

	/* Free table name */
	if (copy_state->table_name != NULL)
	{
		tecc_pfree(copy_state->table_name);
	}

	/* Free schema name */
	if (copy_state->schema_name != NULL)
	{
		tecc_pfree(copy_state->schema_name);
	}

	/* Free column names */
	if (copy_state->column_names != NULL)
	{
		for (i = 0; i < copy_state->column_count; i++)
		{
			if (copy_state->column_names[i] != NULL)
			{
				tecc_pfree(copy_state->column_names[i]);
			}
		}
		tecc_pfree(copy_state->column_names);
	}

	/* Free encrypted column info array */
	if (copy_state->encrypted_column_info != NULL)
	{
		/* Note: Individual TeccEncryptedColumnMetadata structures are managed by cache */
		tecc_pfree(copy_state->encrypted_column_info);
	}

	/* Free column mapping array */
	if (copy_state->column_mapping != NULL)
	{
		tecc_pfree(copy_state->column_mapping);
	}

	/* Free row buffer */
	if (copy_state->row_buffer != NULL)
	{
		tecc_pfree(copy_state->row_buffer);
	}

	/* Free null string */
	if (copy_state->null_string != NULL)
	{
		tecc_pfree(copy_state->null_string);
	}

	/* Free errors array */
	if (copy_state->errors != NULL)
	{
		tecc_pfree(copy_state->errors);
	}

	/* Free processor, crypto and formatter contexts */
	/* Note: Currently these contexts are NULL, but we keep the structure for future use */
	if (copy_state->processor_ctx != NULL)
	{
		/* TODO: Call processor cleanup function when available */
		/* For now, just set to NULL */
		copy_state->processor_ctx = NULL;
	}
	if (copy_state->crypto_ctx != NULL)
	{
		/* Call crypto cleanup function */
		tecc_copy_state_cleanup_crypto_context((TeccEnhancedCopyState *)copy_state);
	}
	if (copy_state->formatter_ctx != NULL)
	{
		/* TODO: Call formatter cleanup function when available */
		/* For now, just set to NULL */
		copy_state->formatter_ctx = NULL;
	}

	/* Free memory pool if allocated */
	if (copy_state->memory_pool != NULL)
	{
		/* Note: Memory pool should be managed by specialized memory manager */
	}

	/* Free COPY state itself */
	tecc_pfree(copy_state);
}

#if 0
/*
 * tecc_parse_copy_command - Parse COPY command
 *
 * Returns: 0 on success, -1 on failure
 */
static int
tecc_parse_copy_command(const char *command, char **table_name,
                        char ***column_names, int *column_count,
                        bool *is_from_stdin, bool *is_to_stdout)
{
	const char *p;
	/* TODO: Call processor module for COPY command parsing */
	/* Processor module should provide: tecc_processor_parse_copy_command() */

	/* For now, return placeholder values for interface compatibility */
	*table_name = NULL;
	*column_names = NULL;
	*column_count = 0;
	*is_from_stdin = false;
	*is_to_stdout = false;

	/* Basic validation */
	if (command == NULL)
	{
		return -1;
	}

	p = command;
	while (*p && (*p == '\0' || *p == '\t' || *p == '\n' || *p == '\r'))
	{
		p++;
	}

	if (strncasecmp(p, "COPY", 4) != 0)
	{
		return -1;
	}

	/* Return success with placeholder values */
	return 0;
}
#endif


/* Begin - ReqID:TECC4.0 - Use frontend parser for COPY command parsing */

/*
 * tecc_parse_copy_with_frontend_parser - Parse COPY command using frontend parser
 *
 * This function uses the frontend parser to parse COPY commands, which provides
 * full support for all COPY syntax including column lists and format options.
 *
 * Returns: 0 on success, -1 on failure
 */
static int
tecc_parse_copy_with_frontend_parser(PGconn *conn, const char *command, char **table_name,
                                     char ***column_names, int *column_count,
                                     bool *is_from_stdin, bool *is_to_stdout,
                                     bool *csv_mode, char *delimiter,
                                     char *quote, char *escape,
                                     bool *header, char **null_string)
{
	Node *parse_tree = NULL;
	ParserError *errors = NULL;
	CopyStmt *copy_stmt = NULL;

	/* Initialize output parameters */
	if (table_name) *table_name = NULL;
	if (column_names) *column_names = NULL;
	if (column_count) *column_count = 0;
	if (is_from_stdin) *is_from_stdin = false;
	if (is_to_stdout) *is_to_stdout = false;
	if (csv_mode) *csv_mode = false;
	if (delimiter) *delimiter = '\t'; /* Default TEXT delimiter */
	if (quote) *quote = '"'; /* Default quote character */
	if (escape) *escape = '\\'; /* Default escape character */
	if (header) *header = false;
	if (null_string) *null_string = NULL;

	/* Basic validation */
	if (command == NULL)
	{
		return -1;
	}

	/* Parse SQL using frontend parser */
	parse_tree = tecc_parse_sql(command, &errors);
	if (parse_tree == NULL || errors != NULL)
	{
		/* Parse failed - log debug info */
		tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
			"tecc_parse_copy: parse failed for command (first 100 chars): '%.100s'",
			command ? command : "(null)");
		if (errors) tecc_error_free(errors);
		return -1;
	}

	/* tecc_parse_sql returns a List* (as Node*). Unwrap the list. */
	if (nodeTag(parse_tree) == FE_T_List)
	{
		List *stmt_list = (List *) parse_tree;
		if (list_length(stmt_list) != 1)
		{
			/* COPY command must be a single statement */
			tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
				"tecc_parse_copy: expected 1 statement, got %d for: '%.100s'",
				list_length(stmt_list), command ? command : "(null)");
			freeNode(parse_tree);
			return -1;
		}
		parse_tree = (Node *) linitial(stmt_list);
	}

	/* Check if it's a COPY statement */
	if (parse_tree == NULL || nodeTag(parse_tree) != FE_T_CopyStmt)
	{
		/* Not a COPY statement */
		tecc_processor_debug_safe(NULL, TECC_LOG_DEBUG1,
			"tecc_parse_copy: unexpected node type %d (expected %d) for: '%.100s'",
			parse_tree ? nodeTag(parse_tree) : -1, FE_T_CopyStmt, command ? command : "(null)");
		freeNode(parse_tree);
		return -1;
	}

	copy_stmt = (CopyStmt *)parse_tree;

	/* ReqID:TECC4.0 — COPY (SELECT ...) TO STDOUT 走后端解密，
	 * 前端解析器无需处理。不报错，直接跳过让后端执行。 */
	if (copy_stmt->relation == NULL && copy_stmt->query != NULL && !copy_stmt->is_from)
	{
		freeNode(parse_tree);
		return 0;  /* Not a client-side COPY — let backend handle it */
	}

	/* Extract table name */
	if (table_name && copy_stmt->relation && copy_stmt->relation->relname)
	{
		*table_name = tecc_strdup(copy_stmt->relation->relname);
		if (*table_name == NULL)
		{
			goto cleanup_error;
		}
	}

	/* Extract column list */
	if (column_names && column_count && copy_stmt->attlist)
	{
		ListCell *lc;
		int i = 0;
		int count = list_length(copy_stmt->attlist);
		*column_names = (char **)pg_malloc(count * sizeof(char *));
		if (*column_names == NULL)
		{
			goto cleanup_error;
		}

		foreach(lc, copy_stmt->attlist)
		{
			ResTarget *restarget = (ResTarget *)lfirst(lc);
			char *col_name = restarget->name;
			(*column_names)[i] = tecc_strdup(col_name);
			if ((*column_names)[i] == NULL)
			{
				/* Cleanup allocated columns */
				for (int j = 0; j < i; j++)
				{
					tecc_pfree((*column_names)[j]);
				}
				tecc_pfree(*column_names);
				*column_names = NULL;
				goto cleanup_error;
			}
			i++;
		}
		*column_count = count;
	}

	/* Extract direction */
	if (is_from_stdin) *is_from_stdin = copy_stmt->is_from;
	if (is_to_stdout) *is_to_stdout = !copy_stmt->is_from;

	/* Extract format options */
	if (copy_stmt->options)
	{
		ListCell *lc;
		foreach(lc, copy_stmt->options)
		{
			DefElem *def = (DefElem *)lfirst(lc);
			if (def->defname == NULL || def->arg == NULL)
			{
				continue;
			}

			if (strcmp(def->defname, "format") == 0)
			{
				char *format = (char *)((Const *)def->arg)->constvalue.sval;
				if (csv_mode && format)
				{
					*csv_mode = (strcasecmp(format, "csv") == 0);
					if (*csv_mode && delimiter)
					{
						*delimiter = ','; /* CSV default delimiter */
					}
				}
			}
			else if (strcmp(def->defname, "delimiter") == 0 && delimiter)
			{
				char *delim = (char *)((Const *)def->arg)->constvalue.sval;
				if (delim && strlen(delim) > 0)
				{
					*delimiter = delim[0];
				}
			}
			else if (strcmp(def->defname, "quote") == 0 && quote)
			{
				char *quote_char = (char *)((Const *)def->arg)->constvalue.sval;
				if (quote_char && strlen(quote_char) > 0)
				{
					*quote = quote_char[0];
				}
			}
			else if (strcmp(def->defname, "escape") == 0 && escape)
			{
				char *escape_char = (char *)((Const *)def->arg)->constvalue.sval;
				if (escape_char && strlen(escape_char) > 0)
				{
					*escape = escape_char[0];
				}
			}
			else if (strcmp(def->defname, "header") == 0 && header)
			{
				char *header_val = (char *)((Const *)def->arg)->constvalue.sval;
				if (header_val)
				{
					*header = (strcasecmp(header_val, "true") == 0);
				}
			}
			else if (strcmp(def->defname, "null") == 0 && null_string)
			{
				char *null_val = (char *)((Const *)def->arg)->constvalue.sval;
				if (null_val)
				{
					*null_string = tecc_strdup(null_val);
					if (*null_string == NULL)
					{
						goto cleanup_error;
					}
				}
			}
		}
	}

	/* Cleanup and return success */
	freeNode(parse_tree);
	return 0;

cleanup_error:
	/* Cleanup allocated resources */
	if (table_name && *table_name)
	{
		tecc_pfree(*table_name);
		*table_name = NULL;
	}

	if (column_names && *column_names && column_count)
	{
		for (int i = 0; i < *column_count; i++)
		{
			tecc_pfree((*column_names)[i]);
		}
		tecc_pfree(*column_names);
		*column_names = NULL;
		*column_count = 0;
	}

	if (null_string && *null_string)
	{
		tecc_pfree(*null_string);
		*null_string = NULL;
	}

	if (parse_tree) freeNode(parse_tree);
	return -1;
}

/* End - ReqID:TECC4.0 - Use frontend parser for COPY command parsing */

/* Begin - ReqID:TECC4.0 - Enhanced COPY row processing */

#if 0
/*
 * tecc_parse_copy_row - Parse a COPY row into fields based on format
 *
 * This function parses a COPY row into fields using the appropriate
 * format parser (CSV or TEXT).
 *
 * Returns: 0 on success, -1 on failure
 */
static int
tecc_parse_copy_row(TeccEnhancedCopyState *copy_state, const char *row_data,
                    int row_len, char ***fields, int **field_lens, int *field_count)
{
	if (copy_state == NULL || row_data == NULL || row_len <= 0 ||
	    fields == NULL || field_lens == NULL || field_count == NULL)
	{
		return -1;
	}

	/* Use appropriate parser based on format */
	if (copy_state->csv_mode)
	{
		/* CSV format parsing */
		return tecc_copy_parse_csv_row(row_data, row_len,
		                               copy_state->delimiter,
		                               copy_state->quote,
		                               copy_state->escape,
		                               fields, field_lens, field_count);
	}
	else
	{
		/* TEXT format parsing */
		return tecc_copy_parse_text_row(row_data, row_len,
		                                copy_state->delimiter,
		                                fields, field_lens, field_count);
	}
}

/*
 * tecc_format_copy_row - Format fields into a COPY row based on format
 *
 * This function formats fields into a COPY row using the appropriate
 * format formatter (CSV or TEXT).
 *
 * Returns: 0 on success, -1 on failure
 */
static int
tecc_format_copy_row(TeccEnhancedCopyState *copy_state, char **fields,
                     int *field_lens, int field_count,
                     char **formatted_row, int *formatted_len)
{
	if (copy_state == NULL || fields == NULL || field_lens == NULL ||
	    field_count <= 0 || formatted_row == NULL || formatted_len == NULL)
	{
		return -1;
	}

	/* Use appropriate formatter based on format */
	if (copy_state->csv_mode)
	{
		/* CSV format formatting */
		return tecc_copy_format_csv_row(fields, field_lens, field_count,
		                                copy_state->delimiter,
		                                copy_state->quote,
		                                copy_state->escape,
		                                formatted_row, formatted_len);
	}
	else
	{
		/* TEXT format formatting */
		return tecc_copy_format_text_row(fields, field_lens, field_count,
		                                 copy_state->delimiter,
		                                 formatted_row, formatted_len);
	}
}


#endif
/*
 * tecc_process_copy_row - Process a single COPY row by calling processor module
 *
 * This function processes a single COPY row by calling the appropriate
 * processor module function for encryption or decryption.
 *
 * Returns: 0 on success, -1 on failure
 */
static int
tecc_process_copy_row(TeccEnhancedCopyState *copy_state, const char *row_data,
                      int row_len, char **processed_row, int *processed_len,
                      bool is_import)
{
	TeccProcessorResult processor_result;

	/* Enhanced input validation */
	if (copy_state == NULL || row_data == NULL || processed_row == NULL || processed_len == NULL)
	{
		/* Log error if possible */
		if (copy_state != NULL && copy_state->conn != NULL)
		{
			tecc_handle_error(copy_state->conn, TECC_ERR_PARAM,
			                  "Invalid parameters in tecc_process_copy_row");
		}
		return -1;
	}

	/* Validate row length */
	if (row_len < 0)
	{
		if (copy_state->conn != NULL)
		{
			tecc_handle_error(copy_state->conn, TECC_ERR_PARAM,
			                  "Invalid row length in tecc_process_copy_row");
		}
		return -1;
	}

	/* Check for empty row */
	if (row_len == 0)
	{
		/* Return empty result for empty row */
		*processed_row = pg_strdup("");
		if (*processed_row == NULL)
		{
			if (copy_state->conn != NULL)
			{
				tecc_handle_error(copy_state->conn, TECC_ERR_MEMORY,
				                  "Failed to allocate empty string for row");
			}
			return -1;
		}
		*processed_len = 0;
		return 0;
	}

	/* Check if there are encrypted columns to process */
	if (copy_state->encrypted_column_count == 0)
	{
		/* No encrypted columns, return original data */
		*processed_row = (char *)row_data;
		*processed_len = row_len;
		return 0;
	}

	/* Call processor module based on operation type */
	if (is_import)
	{
		/* Import: encrypt row data */
		processor_result = tecc_processor_process_copy_row(copy_state, row_data,
		                                                   row_len, processed_row,
		                                                   processed_len);
	}
	else
	{
		/* Export: decrypt row data */
		processor_result = tecc_processor_process_copy_result_row(copy_state, row_data,
		                                                          row_len, processed_row,
		                                                          processed_len);
	}

	/* Check processor result */
	if (processor_result != TECC_PROCESSOR_SUCCESS)
	{
		/* Processor module failed */
		return -1;
	}

	return 0;
}

/*
 * tecc_process_copy_row_stream - 使用流式上下文处理COPY行
 *
 * 参数:
 *   copy_state - COPY状态
 *   row_data - 行数据
 *   row_len - 行长度
 *   processed_row - 处理后的行数据（输出）
 *   processed_len - 处理后的行长度（输出）
 *   is_import - 是否为导入操作
 *
 * 返回:
 *   0 - 成功
 *   -1 - 失败
 */
static int
tecc_process_copy_row_stream(TeccEnhancedCopyState *copy_state, const char *row_data,
                             int row_len, char **processed_row, int *processed_len,
                             bool is_import)
{
    /* Begin - ReqID:TECC4.0 */
    char **fields = NULL;
    int *field_lens = NULL;
    int field_count = 0;
    int i = 0, j;
    int result = -1;
    char *formatted_row = NULL;
    int formatted_len = 0;
    /* 性能优化：创建列索引到加密列信息的映射 */
    TeccEncryptedColumnMetadata **column_index_map = NULL;
    TeccStreamCryptoContext **stream_ctx_map = NULL;

    /* 验证参数 */
    if (copy_state == NULL || row_data == NULL || processed_row == NULL || processed_len == NULL) {
        return -1;
    }

    /* 验证行长度 */
    if (row_len < 0) {
        return -1;
    }

    /* 检查空行 */
    if (row_len == 0) {
        *processed_row = pg_strdup("");
        if (*processed_row == NULL) {
            return -1;
        }
        *processed_len = 0;
        return 0;
    }

    /* 检查是否有加密列 */
    if (copy_state->encrypted_column_count == 0) {
        /* 没有加密列，返回数据的副本 */
        *processed_row = (char *)pg_malloc(row_len + 1);
        if (*processed_row == NULL) {
            return -1;
        }
        memcpy(*processed_row, row_data, row_len);
        (*processed_row)[row_len] = '\0';
        *processed_len = row_len;
        return 0;
    }

    /* 检查流式上下文是否已初始化 */
    if (!copy_state->stream_contexts_initialized) {
        /* 流式上下文未初始化，回退到标准处理 */
        return tecc_process_copy_row(copy_state, row_data, row_len,
                                     processed_row, processed_len, is_import);
    }

    /* 解析行数据为字段 */
    if (copy_state->csv_mode) {
        result = tecc_copy_parse_csv_row(row_data, row_len,
                                        copy_state->delimiter,
                                        copy_state->quote,
                                        copy_state->escape,
                                        &fields, &field_lens, &field_count);
    } else {
        result = tecc_copy_parse_text_row(row_data, row_len,
                                         copy_state->delimiter,
                                         &fields, &field_lens, &field_count);
    }

    if (result != 0 || fields == NULL) {
        /* 解析失败 */
        return -1;
    }

    /* 性能优化：创建列索引到加密列信息的映射 */
    if (field_count > 0 && copy_state->encrypted_column_count > 0) {
        column_index_map = (TeccEncryptedColumnMetadata **)tecc_palloc(field_count * sizeof(TeccEncryptedColumnMetadata *));
        stream_ctx_map = (TeccStreamCryptoContext **)tecc_palloc(field_count * sizeof(TeccStreamCryptoContext *));

        if (column_index_map == NULL || stream_ctx_map == NULL) {
            /* 内存分配失败，回退到原始逻辑 */
            if (column_index_map != NULL) tecc_pfree(column_index_map);
            if (stream_ctx_map != NULL) tecc_pfree(stream_ctx_map);
            column_index_map = NULL;
            stream_ctx_map = NULL;
        } else {
            /* 清零内存，模拟calloc行为 */
            memset(column_index_map, 0, field_count * sizeof(TeccEncryptedColumnMetadata *));
            memset(stream_ctx_map, 0, field_count * sizeof(TeccStreamCryptoContext *));

            /* 填充映射 */
            for (j = 0; j < copy_state->encrypted_column_count; j++) {
            int field_index;
                TeccEncryptedColumnMetadata *col_info = copy_state->encrypted_column_info[j];
                TeccStreamCryptoContext *stream_ctx =
                    ((TeccStreamCryptoContext **)copy_state->stream_crypto_contexts)[j];

                if (col_info == NULL || stream_ctx == NULL) {
                    continue;
                }

                /* 确定字段索引 */
            field_index = -1;
                if (copy_state->column_mapping != NULL && i < copy_state->mapping_count) {
                    /* 使用列映射查找字段索引 */
                    for (int k = 0; k < field_count; k++) {
                        if (copy_state->column_mapping[k] == col_info->column_attnum) {
                            field_index = k;
                            break;
                        }
                    }
                } else {
                    /* 直接使用列索引 */
                    if (col_info->column_attnum >= 0 && col_info->column_attnum < field_count) {
                        field_index = col_info->column_attnum;
                    }
                }

                if (field_index >= 0 && field_index < field_count) {
                    column_index_map[field_index] = col_info;
                    stream_ctx_map[field_index] = stream_ctx;
                }
            }
        }
    }

    /* 处理每个字段 */
    for (i = 0; i < field_count; i++) {
        /* 使用映射检查当前字段是否对应加密列 */
        TeccEncryptedColumnMetadata *col_info = NULL;
        TeccStreamCryptoContext *stream_ctx = NULL;

        if (column_index_map != NULL) {
            /* 使用优化映射 */
            col_info = column_index_map[i];
            stream_ctx = stream_ctx_map[i];
        } else {
            /* 回退到原始嵌套循环 */
            for (j = 0; j < copy_state->encrypted_column_count; j++) {
                col_info = copy_state->encrypted_column_info[j];
                stream_ctx = ((TeccStreamCryptoContext **)copy_state->stream_crypto_contexts)[j];

                if (col_info == NULL || stream_ctx == NULL) {
                    continue;
                }

                /* 检查列映射 */
                if (copy_state->column_mapping != NULL) {
                    /* 使用列映射 */
                    int mapped_index = copy_state->column_mapping[i];
                    if (mapped_index != col_info->column_attnum) {
                        col_info = NULL;
                        stream_ctx = NULL;
                        continue;
                    }
                } else {
                    /* 直接使用字段索引 */
                    if (i != col_info->column_attnum) {
                        col_info = NULL;
                        stream_ctx = NULL;
                        continue;
                    }
                }

                /* 找到匹配的列 */
                break;
            }
        }

        if (col_info == NULL || stream_ctx == NULL) {
            /* 当前字段不是加密列，跳过 */
            continue;
        }

        /* 处理加密列 */
            if (is_import) {
                /* 导入：加密字段 */
                TeccUnifiedEncryptResult *encrypt_result;

                encrypt_result = tecc_stream_encrypt_data(stream_ctx,
                                                         fields[i],
                                                         field_lens[i]);

                if (encrypt_result != NULL &&
                    encrypt_result->error_code == TECC_CRYPTO_SUCCESS &&
                    encrypt_result->hex_string != NULL) {
                    /* 更新字段为加密后的十六进制字符串 */
                    if (fields[i] != row_data) { /* 不要释放原始缓冲区 */
                        tecc_pfree(fields[i]);
                    }
                    fields[i] = encrypt_result->hex_string;
                    field_lens[i] = encrypt_result->hex_len;

                    /* 释放结果结构体但不释放hex_string */
                    encrypt_result->hex_string = NULL;
                    tecc_unified_free_encrypt_result(encrypt_result);
                } else {
                    /* 加密失败 */
                    if (encrypt_result != NULL) {
                        tecc_unified_free_encrypt_result(encrypt_result);
                    }
                    tecc_copy_free_parsed_fields(fields, field_lens, field_count);
                    /* 清理映射内存 */
                    if (column_index_map != NULL) {
                        tecc_pfree(column_index_map);
                    }
                    if (stream_ctx_map != NULL) {
                        tecc_pfree(stream_ctx_map);
                    }
                    return -1;
                }
            } else {
                /* 导出：解密字段 */
                TeccUnifiedDecryptResult *decrypt_result;

                decrypt_result = tecc_stream_decrypt_data(stream_ctx,
                                                         fields[i],
                                                         field_lens[i]);

                if (decrypt_result != NULL &&
                    decrypt_result->error_code == TECC_CRYPTO_SUCCESS &&
                    decrypt_result->plaintext != NULL) {
                    /* 更新字段为解密后的明文 */
                    if (fields[i] != row_data) { /* 不要释放原始缓冲区 */
                        tecc_pfree(fields[i]);
                    }
                    fields[i] = decrypt_result->plaintext;
                    field_lens[i] = decrypt_result->plain_len;

                    /* 释放结果结构体但不释放plaintext */
                    decrypt_result->plaintext = NULL;
                    tecc_unified_free_decrypt_result(decrypt_result);
                } else {
                    /* 解密失败 */
                    if (decrypt_result != NULL) {
                        tecc_unified_free_decrypt_result(decrypt_result);
                    }
                    tecc_copy_free_parsed_fields(fields, field_lens, field_count);
                    /* 清理映射内存 */
                    if (column_index_map != NULL) {
                        tecc_pfree(column_index_map);
                    }
                    if (stream_ctx_map != NULL) {
                        tecc_pfree(stream_ctx_map);
                    }
                    return -1;
                }
            }
    }

    /* 将字段重新格式化为行 */
    if (copy_state->csv_mode) {
        result = tecc_copy_format_csv_row(fields, field_lens, field_count,
                                         copy_state->delimiter,
                                         copy_state->quote,
                                         copy_state->escape,
                                         &formatted_row, &formatted_len);
    } else {
        result = tecc_copy_format_text_row(fields, field_lens, field_count,
                                          copy_state->delimiter,
                                          &formatted_row, &formatted_len);
    }

    /* 释放解析的字段（但不释放字段数据，因为它们已经被重新分配） */
    if (fields != NULL) {
        tecc_pfree(fields);
    }
    if (field_lens != NULL) {
        tecc_pfree(field_lens);
    }

    /* 清理映射内存 */
    if (column_index_map != NULL) {
        tecc_pfree(column_index_map);
    }
    if (stream_ctx_map != NULL) {
        tecc_pfree(stream_ctx_map);
    }

    if (result != 0 || formatted_row == NULL) {
        return -1;
    }

    *processed_row = formatted_row;
    *processed_len = formatted_len;
    return 0;
    /* End - ReqID:TECC4.0 */
}

/* End - ReqID:TECC4.0 */
/* ==================== COPY State Helper Functions ==================== */

/*
 * tecc_copy_state_load_encrypted_columns - 加载加密列信息
 *
 * 参数:
 *   copy_state - COPY状态
 *
 * 返回:
 *   0 - 成功
 *   -1 - 失败
 */
int
tecc_copy_state_load_encrypted_columns(TeccEnhancedCopyState *copy_state)
{
	TeccState *state;
	const char *database_name;
	const char *schema_name;
	int i, j;
	int encrypted_count = 0;
	TeccEncryptedColumnMetadata **encrypted_columns = NULL;
	TeccColumnCacheItem **cached_columns = NULL;
	int cached_column_count = 0;
	/* 性能优化：创建列名到缓存项的映射 */
	TeccColumnCacheItem **column_name_map = NULL;
	int column_name_map_size = 0;

	state = NULL;
	if (copy_state == NULL || copy_state->table_name == NULL)
	{
		return -1;
	}

	/* 获取TECC状态 */
	if (copy_state->conn == NULL)
	{
		/* Cannot get state when no connection, use NULL */
		return -1;
	}

	state = tecc_get_state(copy_state->conn);
	if (state == NULL)
	{
		/* Cannot get state when TECC not initialized, use NULL */
		return -1;
	}

	/* 添加调试日志 */
	tecc_record_debug(state, "Loading encrypted columns for table: %s", copy_state->table_name);

	/* 从缓存获取表中所有加密列 */
	/* 获取数据库名和模式名 */
	database_name = NULL;
	schema_name = NULL;

	/* 尝试从连接获取数据库名 */
	if (copy_state->conn != NULL) {
		database_name = PQdb(copy_state->conn);
	}

	/* 使用COPY状态中的模式名，如果未设置则使用默认值 */
	if (copy_state->schema_name != NULL && copy_state->schema_name[0] != '\0') {
		schema_name = copy_state->schema_name;
	} else {
		schema_name = "public";  /* 默认模式名 */
	}

	/* 如果数据库名为空，使用默认值 */
	if (database_name == NULL || database_name[0] == '\0') {
		database_name = "postgres";  /* 默认数据库名 */
	}

	cached_columns = tecc_cache_get_columns_by_table_name(state, database_name,
														 schema_name, copy_state->table_name,
														 &cached_column_count);

	if (cached_columns == NULL || cached_column_count == 0)
	{
		/* 表中没有加密列，正常返回 */
		copy_state->encrypted_column_info = NULL;
		copy_state->encrypted_column_count = 0;
		tecc_record_debug(state, "No encrypted columns found in cache for table: %s", copy_state->table_name);
		return 0;
	}

	/* 性能优化：创建列名到缓存项的映射 */
	column_name_map_size = cached_column_count;
	column_name_map = (TeccColumnCacheItem **)pg_malloc(column_name_map_size * sizeof(TeccColumnCacheItem *));
	if (column_name_map == NULL)
	{
		tecc_record_debug(state, "Failed to allocate memory for column name map for table: %s", copy_state->table_name);
		/* 注意：这里不释放cached_columns，因为缓存模块管理其生命周期 */
		return -1;
	}

	/* 初始化映射数组 */
	for (j = 0; j < column_name_map_size; j++)
	{
		column_name_map[j] = NULL;
	}

	/* 填充列名映射 */
	for (j = 0; j < column_name_map_size; j++)
	{
		column_name_map[j] = cached_columns[j];
	}

	/* 计算需要的加密列数量 */
	if (copy_state->column_count == 0)
	{
		encrypted_count = column_name_map_size;
	}
	else
	{
		for (i = 0; i < copy_state->column_count; i++)
		{
			const char *column_name = copy_state->column_names[i];
			if (column_name != NULL)
			{
				/* 在缓存列中查找匹配的列 */
				for (j = 0; j < column_name_map_size; j++)
				{
					if (column_name_map[j] != NULL &&
						column_name_map[j]->column_name != NULL &&
						strcmp(column_name_map[j]->column_name, column_name) == 0)
					{
						encrypted_count++;
						break;
					}
				}
			}
		}
	}

	if (encrypted_count == 0)
	{
		/* COPY语句指定的列中没有加密列，正常返回 */
		copy_state->encrypted_column_info = NULL;
		copy_state->encrypted_column_count = 0;
		tecc_record_debug(state, "No encrypted columns in COPY column list for table: %s", copy_state->table_name);
		/* 注意：这里不释放cached_columns，因为缓存模块管理其生命周期 */
		return 0;
	}

	/* 分配加密列信息数组 */
	encrypted_columns = (TeccEncryptedColumnMetadata **)pg_malloc(
		encrypted_count * sizeof(TeccEncryptedColumnMetadata *));
	if (encrypted_columns == NULL)
	{
		tecc_record_debug(state, "Failed to allocate memory for encrypted column array for table: %s", copy_state->table_name);
		/* 清理列名映射内存 */
		if (column_name_map != NULL)
		{
			tecc_pfree(column_name_map);
			column_name_map = NULL;
		}
		return -1;
	}

		/* 填充加密列信息 */
	encrypted_count = 0;
	if (copy_state->column_count == 0)
	{
		for (j = 0; j < column_name_map_size; j++)
		{
			if (column_name_map[j] != NULL && column_name_map[j]->column_name != NULL)
			{
				TeccEncryptedColumnMetadata *column_meta = (TeccEncryptedColumnMetadata *)pg_malloc(sizeof(TeccEncryptedColumnMetadata));
				if (column_meta == NULL)
				{
					tecc_record_debug(state, "Failed to allocate memory for column metadata for table: %s", copy_state->table_name);
					for (int k = 0; k < encrypted_count; k++)
						if (encrypted_columns[k] != NULL) tecc_pfree(encrypted_columns[k]);
					tecc_pfree(encrypted_columns);
					if (column_name_map != NULL) { tecc_pfree(column_name_map); column_name_map = NULL; }
					return -1;
				}
				memset(column_meta, 0, sizeof(TeccEncryptedColumnMetadata));
				column_meta->table_oid = column_name_map[j]->table_oid;
				column_meta->column_attnum = column_name_map[j]->column_attnum;
				column_meta->column_name = pg_strdup(column_name_map[j]->column_name);
				column_meta->data_type_original_oid = column_name_map[j]->original_type_oid;
				column_meta->data_type_original_mod = column_name_map[j]->original_type_mod;
				column_meta->dek_name = NULL;
				if (state != NULL && column_name_map[j]->dek_id != InvalidOid) {
					TeccDekCacheItem *dek_item = tecc_cache_get_dek_by_oid(state, column_name_map[j]->dek_id);
					if (dek_item == NULL && column_name_map[j]->dek_name != NULL)
						dek_item = tecc_cache_get_dek_by_name_search_path(state, column_name_map[j]->dek_name);
					if (dek_item != NULL && dek_item->key_name != NULL)
						column_meta->dek_name = pg_strdup(dek_item->key_name);
				}
				column_meta->encryption_type = column_name_map[j]->encryption_type;
				column_meta->column_key_id = column_name_map[j]->dek_id;
				column_meta->enable_det = (column_name_map[j]->encryption_type & 0x01) != 0;
				column_meta->enable_ope = (column_name_map[j]->encryption_type & 0x02) != 0;
				column_meta->enable_search = (column_name_map[j]->encryption_type & 0x04) != 0;
				column_meta->collation_oid = column_name_map[j]->collation_oid;
				column_meta->collation_locale = column_name_map[j]->collation_locale ? pg_strdup(column_name_map[j]->collation_locale) : NULL;
				column_meta->create_date = column_name_map[j]->create_date;
				column_meta->creator = 0;
				encrypted_columns[encrypted_count] = column_meta;
				encrypted_count++;
			}
		}
		copy_state->encrypted_column_info = encrypted_columns;
		copy_state->encrypted_column_count = encrypted_count;
		if (column_name_map != NULL) { tecc_pfree(column_name_map); column_name_map = NULL; }
		return 0;
	}
	else
	{
		for (i = 0; i < copy_state->column_count; i++)
		{
			const char *column_name = copy_state->column_names[i];
			if (column_name != NULL)
			{
				/* 在缓存列中查找匹配的列 */
				for (j = 0; j < column_name_map_size; j++)
				{
					if (column_name_map[j] != NULL &&
						column_name_map[j]->column_name != NULL &&
						strcmp(column_name_map[j]->column_name, column_name) == 0)
					{
						TeccEncryptedColumnMetadata *column_meta = (TeccEncryptedColumnMetadata *)pg_malloc(sizeof(TeccEncryptedColumnMetadata));
						if (column_meta == NULL)
						{
							tecc_record_debug(state, "Failed to allocate memory for column metadata for table: %s", copy_state->table_name);
							for (int k = 0; k < encrypted_count; k++)
								if (encrypted_columns[k] != NULL) tecc_pfree(encrypted_columns[k]);
							tecc_pfree(encrypted_columns);
							if (column_name_map != NULL) { tecc_pfree(column_name_map); column_name_map = NULL; }
							return -1;
						}
						memset(column_meta, 0, sizeof(TeccEncryptedColumnMetadata));
						column_meta->table_oid = column_name_map[j]->table_oid;
						column_meta->column_attnum = column_name_map[j]->column_attnum;
						column_meta->column_name = pg_strdup(column_name_map[j]->column_name);
						column_meta->data_type_original_oid = column_name_map[j]->original_type_oid;
						column_meta->data_type_original_mod = column_name_map[j]->original_type_mod;
						column_meta->dek_name = NULL;
						if (state != NULL && column_name_map[j]->dek_id != InvalidOid) {
							TeccDekCacheItem *dek_item = tecc_cache_get_dek_by_oid(state, column_name_map[j]->dek_id);
							if (dek_item == NULL && column_name_map[j]->dek_name != NULL)
								dek_item = tecc_cache_get_dek_by_name_search_path(state, column_name_map[j]->dek_name);
							if (dek_item != NULL && dek_item->key_name != NULL)
								column_meta->dek_name = pg_strdup(dek_item->key_name);
						}
						column_meta->encryption_type = column_name_map[j]->encryption_type;
						column_meta->column_key_id = column_name_map[j]->dek_id;
						column_meta->enable_det = (column_name_map[j]->encryption_type & 0x01) != 0;
						column_meta->enable_ope = (column_name_map[j]->encryption_type & 0x02) != 0;
						column_meta->enable_search = (column_name_map[j]->encryption_type & 0x04) != 0;
						column_meta->collation_oid = column_name_map[j]->collation_oid;
						column_meta->collation_locale = column_name_map[j]->collation_locale ? pg_strdup(column_name_map[j]->collation_locale) : NULL;
						column_meta->create_date = column_name_map[j]->create_date;
						column_meta->creator = 0;
						encrypted_columns[encrypted_count] = column_meta;
						encrypted_count++;
						break;
					}
				}
			}
		}
	}
	/* 更新COPY状态 */
	copy_state->encrypted_column_info = encrypted_columns;
	copy_state->encrypted_column_count = encrypted_count;

	/* 记录调试信息 */
	if (encrypted_count > 0)
	{
		char debug_msg[256];
		snprintf(debug_msg, sizeof(debug_msg),
				"Loaded %d encrypted columns for table %s from cache",
				encrypted_count, copy_state->table_name);
		tecc_record_debug(state, "%s", debug_msg);
	}

	/* 清理列名映射内存 */
	if (column_name_map != NULL)
	{
		tecc_pfree(column_name_map);
		column_name_map = NULL;
	}

	return 0;
}

/*
 * tecc_copy_state_init_column_mapping - 初始化列映射
 *
 * 参数:
 *   copy_state - COPY状态
 *
 * 返回:
 *   0 - 成功
 *   -1 - 失败
 */
int
tecc_copy_state_init_column_mapping(TeccEnhancedCopyState *copy_state)
{
	int i;
	int *column_mapping = NULL;

	if (copy_state == NULL || copy_state->column_count <= 0)
	{
		return -1;
	}

	/* 分配列映射数组 */
	column_mapping = (int *)pg_malloc(copy_state->column_count * sizeof(int));
	if (column_mapping == NULL)
	{
		return -1;
	}

	/* 初始化列映射：默认情况下，文件列i映射到表列i */
	for (i = 0; i < copy_state->column_count; i++)
	{
		column_mapping[i] = i;
	}

	/* 更新COPY状态 */
	copy_state->column_mapping = column_mapping;
	copy_state->mapping_count = copy_state->column_count;

	/* 记录调试信息 */
	tecc_record_debug(NULL, "Initialized column mapping (identity mapping)");

	return 0;
}

/*
 * tecc_copy_state_init_processor_context - 初始化处理器上下文
 *
 * 参数:
 *   copy_state - COPY状态
 *
 * 返回:
 *   0 - 成功
 *   -1 - 失败
 */
int
tecc_copy_state_init_processor_context(TeccEnhancedCopyState *copy_state)
{
	/* 目前处理器上下文初始化为NULL，未来可以添加实际的初始化逻辑 */
	copy_state->processor_ctx = NULL;

	/* 记录调试信息 */
	tecc_record_debug(NULL, "Initialized processor context (placeholder)");

	return 0;
}

/*
 * tecc_copy_state_init_crypto_context - 初始化加密上下文
 *
 * 参数:
 *   copy_state - COPY状态
 *
 * 返回:
 *   0 - 成功
 *   -1 - 失败
 */
int
tecc_copy_state_init_crypto_context(TeccEnhancedCopyState *copy_state)
{
	TeccState *state;
	/* Begin - ReqID:TECC4.0 */
	TeccCryptoContext **crypto_contexts = NULL;
	int i;

	/* 验证参数 */
	if (copy_state == NULL) {
		return -1;
	}

	/* 检查是否有加密列 */
	if (copy_state->encrypted_column_count == 0) {
		/* 没有加密列，不需要初始化加密上下文 */
		copy_state->crypto_ctx = NULL;
		return 0;
	}

	/* 为每个加密列分配加密上下文数组 */
	crypto_contexts = (TeccCryptoContext **)pg_malloc(
		sizeof(TeccCryptoContext *) * copy_state->encrypted_column_count);
	if (crypto_contexts == NULL) {
		state = (copy_state->conn != NULL) ? tecc_get_state(copy_state->conn) : NULL;
		tecc_record_debug(state,
			"Failed to allocate crypto contexts array for %d encrypted columns",
			copy_state->encrypted_column_count);
		return -1;
	}

	/* 初始化数组为NULL */
	for (i = 0; i < copy_state->encrypted_column_count; i++) {
		crypto_contexts[i] = NULL;
	}

	/* 为每个加密列创建加密上下文 */
	for (i = 0; i < copy_state->encrypted_column_count; i++) {
		TeccAlgorithmSupport algo_support;
		TeccEncryptedColumnMetadata *col_info = copy_state->encrypted_column_info[i];
		char *kek_name = NULL;
		TeccAlgorithmType algo_type = TECC_ALGO_RND; /* 默认算法 */
		int ret;

		if (col_info == NULL) {
			state = (copy_state->conn != NULL) ? tecc_get_state(copy_state->conn) : NULL;
		tecc_record_debug(state,
				"Encrypted column info is NULL at index %d", i);
			continue;
		}

		/* 获取KEK名称 */
		if (col_info->dek_name != NULL) {
			ret = tecc_processor_get_kek_name_for_dek(copy_state->conn,
			                                         col_info->dek_name,
			                                         &kek_name);
			if (ret != 0 || kek_name == NULL) {
				state = (copy_state->conn != NULL) ? tecc_get_state(copy_state->conn) : NULL;
		tecc_record_debug(state,
					"Failed to get KEK name for DEK %s (ret: %d), aborting",
					col_info->dek_name, ret);
				/* KEK名称解析失败，清理并中止 */
				for (int j = 0; j < i; j++) {
					if (crypto_contexts[j] != NULL) {
						tecc_destroy_crypto_context(crypto_contexts[j]);
					}
				}
				tecc_pfree(crypto_contexts);
				return -1;
			}
		} else {
			state = (copy_state->conn != NULL) ? tecc_get_state(copy_state->conn) : NULL;
			tecc_record_debug(state,
				"DEK name is NULL for column %s.%s, cannot resolve KEK",
				copy_state->table_name, col_info->column_name);
			for (int j = 0; j < i; j++) {
				if (crypto_contexts[j] != NULL) {
					tecc_destroy_crypto_context(crypto_contexts[j]);
				}
			}
			tecc_pfree(crypto_contexts);
			return -1;
		}

		/* 创建加密上下文 */
		crypto_contexts[i] = tecc_create_crypto_context_with_pool(
			copy_state->table_name,
			col_info->column_name,
			kek_name,  /* 使用获取的KEK名称 */
			col_info->dek_name);

		/* 释放KEK名称内存 */
		if (kek_name != NULL) {
			tecc_pfree(kek_name);
			kek_name = NULL;
		}

		if (crypto_contexts[i] == NULL) {
			state = (copy_state->conn != NULL) ? tecc_get_state(copy_state->conn) : NULL;
		tecc_record_debug(state,
				"Failed to create crypto context for column %s.%s",
				copy_state->table_name, col_info->column_name);

			/* 清理已分配的上下文 */
			for (int j = 0; j < i; j++) {
				if (crypto_contexts[j] != NULL) {
					tecc_destroy_crypto_context(crypto_contexts[j]);
				}
			}
			tecc_pfree(crypto_contexts);
			return -1;
		}

		/* 设置算法支持 */
		memset(&algo_support, 0, sizeof(algo_support));

		/* 根据加密类型位掩码设置算法支持 */
		/* encryption_type位掩码: 0x01=DET, 0x02=OPE, 0x04=SEARCH */
		if (col_info->encryption_type & 0x01) { /* DET */
			algo_support.supports_det = true;
			algo_type = TECC_ALGO_DET;
		}
		if (col_info->encryption_type & 0x02) { /* OPE */
			algo_support.supports_ope = true;
			algo_type = TECC_ALGO_OPE;
		}
		if (col_info->encryption_type & 0x04) { /* SEARCH */
			algo_support.supports_search = true;
			algo_type = TECC_ALGO_SEARCH;
		}

		/* 如果没有设置任何算法，默认支持RND */
		if (!algo_support.supports_det && !algo_support.supports_ope &&
		    !algo_support.supports_search) {
			algo_support.supports_rnd = true;
			algo_type = TECC_ALGO_RND;
		}

		/* 更新上下文中的算法支持 */
		crypto_contexts[i]->core.algo_support = algo_support;
		crypto_contexts[i]->core.selected_algorithms = algo_type;

		/* 设置排序规则（如果适用） */
		if (col_info->collation_oid != InvalidOid) {
			crypto_contexts[i]->core.collation_oid = col_info->collation_oid;
		}

		state = (copy_state->conn != NULL) ? tecc_get_state(copy_state->conn) : NULL;
		tecc_record_debug(state,
			"Created crypto context for column %s.%s (enc_type: 0x%x, algo: %d)",
			copy_state->table_name, col_info->column_name,
			col_info->encryption_type, algo_type);
	}

	/* 将加密上下文数组保存到COPY状态 */
	copy_state->crypto_ctx = crypto_contexts;

	state = (copy_state->conn != NULL) ? tecc_get_state(copy_state->conn) : NULL;
	tecc_record_debug(state,
		"Initialized crypto contexts for %d encrypted columns",
		copy_state->encrypted_column_count);

	return 0;
	/* End - ReqID:TECC4.0 */
}

/*
 * tecc_copy_state_cleanup_crypto_context - 清理加密上下文
 *
 * 参数:
 *   copy_state - COPY状态
 *
 * 返回:
 *   无
 */
void
tecc_copy_state_cleanup_crypto_context(TeccEnhancedCopyState *copy_state)
{
	TeccState *state;
	/* Begin - ReqID:TECC4.0 */
	TeccCryptoContext **crypto_contexts;
	int i;

	/* 验证参数 */
	if (copy_state == NULL || copy_state->crypto_ctx == NULL) {
		return;
	}

	crypto_contexts = (TeccCryptoContext **)copy_state->crypto_ctx;

	/* 销毁每个加密上下文 */
	for (i = 0; i < copy_state->encrypted_column_count; i++) {
		if (crypto_contexts[i] != NULL) {
			tecc_destroy_crypto_context(crypto_contexts[i]);
			crypto_contexts[i] = NULL;
		}
	}

	/* 释放上下文数组 */
	tecc_pfree(crypto_contexts);
	copy_state->crypto_ctx = NULL;

	state = (copy_state->conn != NULL) ? tecc_get_state(copy_state->conn) : NULL;
	tecc_record_debug(state,
		"Cleaned up crypto contexts for %d encrypted columns",
		copy_state->encrypted_column_count);
	/* End - ReqID:TECC4.0 */
}

/*
 * tecc_copy_state_init_stream_contexts - 初始化流式加密上下文
 *
 * 参数:
 *   copy_state - COPY状态
 *
 * 返回:
 *   0 - 成功
 *   -1 - 失败
 */
int
tecc_copy_state_init_stream_contexts(TeccEnhancedCopyState *copy_state)
{
	TeccState *state;
    /* Begin - ReqID:TECC4.0 */
    TeccStreamCryptoContext **stream_contexts = NULL;
    int i;

    /* 验证参数 */
    if (copy_state == NULL) {
        return -1;
    }

    /* 检查是否启用了流式优化 */
    if (!copy_state->stream_optimized) {
        /* 流式优化未启用，不需要初始化流式上下文 */
        copy_state->stream_contexts_initialized = false;
        return 0;
    }

    /* 检查是否有加密列 */
    if (copy_state->encrypted_column_count == 0) {
        /* 没有加密列，不需要初始化流式上下文 */
        copy_state->stream_contexts_initialized = false;
        return 0;
    }

    /* 为每个加密列分配流式上下文数组 */
    stream_contexts = (TeccStreamCryptoContext **)pg_malloc(
        sizeof(TeccStreamCryptoContext *) * copy_state->encrypted_column_count);
    if (stream_contexts == NULL) {
        state = (copy_state->conn != NULL) ? tecc_get_state(copy_state->conn) : NULL;
    tecc_record_debug(state,
            "Failed to allocate stream contexts array for %d encrypted columns",
            copy_state->encrypted_column_count);
        return -1;
    }

    /* 初始化数组为NULL */
    for (i = 0; i < copy_state->encrypted_column_count; i++) {
        stream_contexts[i] = NULL;
    }

    /* 为每个加密列创建流式加密上下文 */
    for (i = 0; i < copy_state->encrypted_column_count; i++) {
        TeccEncryptedColumnMetadata *col_info = copy_state->encrypted_column_info[i];
        char *kek_name = NULL;
        int ret;

        if (col_info == NULL) {
            state = (copy_state->conn != NULL) ? tecc_get_state(copy_state->conn) : NULL;
            tecc_record_debug(state,
                "Encrypted column info is NULL at index %d", i);
            continue;
        }

        /* 获取KEK名称 */
        if (col_info->dek_name != NULL) {
            ret = tecc_processor_get_kek_name_for_dek(copy_state->conn,
                                                     col_info->dek_name,
                                                     &kek_name);
            if (ret != 0 || kek_name == NULL) {
                state = (copy_state->conn != NULL) ? tecc_get_state(copy_state->conn) : NULL;
            tecc_record_debug(state,
                    "Failed to get KEK name for DEK %s, column %s",
                    col_info->dek_name, col_info->column_name);
                /* 继续处理其他列 */
                continue;
            }
        } else {
            state = (copy_state->conn != NULL) ? tecc_get_state(copy_state->conn) : NULL;
            tecc_record_debug(state,
                "DEK name is NULL for encrypted column %s",
                col_info->column_name);
            continue;
        }

        /* 创建流式加密上下文 */
        stream_contexts[i] = tecc_create_stream_crypto_context_with_conn(
            copy_state->conn,
            copy_state->table_name,
            col_info->column_name,
            kek_name,
            col_info->dek_name);

        if (stream_contexts[i] == NULL) {
            state = (copy_state->conn != NULL) ? tecc_get_state(copy_state->conn) : NULL;
            tecc_record_debug(state,
                "Failed to create stream crypto context for column %s",
                col_info->column_name);
            /* 继续处理其他列 */
        }

        /* 设置算法类型 */
        if (stream_contexts[i] != NULL) {
            int algorithm_type = TECC_ALGO_RND; /* 默认使用RND算法 */

            /* 根据加密类型位掩码确定算法类型 */
            /* encryption_type位掩码: 0x01=DET, 0x02=OPE, 0x04=SEARCH */
            if (col_info->encryption_type & 0x01) { /* DET */
                algorithm_type = TECC_ALGO_DET;
            } else if (col_info->encryption_type & 0x02) { /* OPE */
                algorithm_type = TECC_ALGO_OPE;
            } else if (col_info->encryption_type & 0x04) { /* SEARCH */
                algorithm_type = TECC_ALGO_SEARCH;
            }

            tecc_stream_set_algorithm_type(stream_contexts[i], algorithm_type);
        }

        /* 释放KEK名称内存 */
        if (kek_name != NULL) {
            tecc_pfree(kek_name);
        }
    }

    /* 设置COPY状态中的流式上下文 */
    copy_state->stream_crypto_contexts = (void **)stream_contexts;
    copy_state->stream_context_count = copy_state->encrypted_column_count;
    copy_state->stream_contexts_initialized = true;

    state = (copy_state->conn != NULL) ? tecc_get_state(copy_state->conn) : NULL;
    tecc_record_debug(state,
        "Initialized stream contexts for %d encrypted columns",
        copy_state->encrypted_column_count);

    return 0;
    /* End - ReqID:TECC4.0 */
}

/*
 * tecc_copy_state_cleanup_stream_contexts - 清理流式加密上下文
 *
 * 参数:
 *   copy_state - COPY状态
 *
 * 返回:
 *   0 - 成功
 *   -1 - 失败
 */
int
tecc_copy_state_cleanup_stream_contexts(TeccEnhancedCopyState *copy_state)
{
	TeccState *state;
    /* Begin - ReqID:TECC4.0 */
    TeccStreamCryptoContext **stream_contexts;
    int i;

    /* 验证参数 */
    if (copy_state == NULL || !copy_state->stream_contexts_initialized) {
        return 0;
    }

    stream_contexts = (TeccStreamCryptoContext **)copy_state->stream_crypto_contexts;

    /* 销毁每个流式上下文 */
    for (i = 0; i < copy_state->stream_context_count; i++) {
        if (stream_contexts[i] != NULL) {
            tecc_stream_destroy_context(stream_contexts[i]);
            stream_contexts[i] = NULL;
        }
    }

    /* 释放流式上下文数组 */
    if (stream_contexts != NULL) {
        tecc_pfree(stream_contexts);
        copy_state->stream_crypto_contexts = NULL;
    }

    /* 重置状态 */
    copy_state->stream_context_count = 0;
    copy_state->stream_contexts_initialized = false;

    state = (copy_state->conn != NULL) ? tecc_get_state(copy_state->conn) : NULL;
    tecc_record_debug(state, "Cleaned up stream contexts");
    return 0;
    /* End - ReqID:TECC4.0 */
}

/* Begin - ReqID:TECC4.0 - CSV/TEXT format parsing helper functions */

/*
 * tecc_copy_parse_csv_row - Parse a CSV format row into fields
 *
 * This function parses a CSV format row according to PostgreSQL COPY CSV rules.
 *
 * Parameters:
 *   row_data - Input row data
 *   row_len - Length of row data
 *   delimiter - Field delimiter (default ',')
 *   quote - Quote character (default '"')
 *   escape - Escape character (default '"' for CSV mode)
 *   fields - Output array for field pointers (allocated by this function)
 *   field_lens - Output array for field lengths (allocated by this function)
 *   field_count - Output number of fields found
 *
 * Returns:
 *   0 - Success
 *   -1 - Error
 */
static int
tecc_copy_parse_csv_row(const char *row_data, int row_len,
                        char delimiter, char quote, char escape,
                        char ***fields, int **field_lens, int *field_count)
{
	const char *pos = row_data;
	const char *end = row_data + row_len;
	char **field_array = NULL;
	int *len_array = NULL;
	int max_fields = 10;
	int field_idx = 0;
	int in_quotes = 0;
	const char *field_start = NULL;

	if (row_data == NULL || row_len <= 0 || fields == NULL ||
	    field_lens == NULL || field_count == NULL)
	{
		return -1;
	}

	/* Allocate initial arrays */
	field_array = (char **)pg_malloc(max_fields * sizeof(char *));
	len_array = (int *)pg_malloc(max_fields * sizeof(int));
	if (field_array == NULL || len_array == NULL)
	{
		if (field_array) tecc_pfree(field_array);
		if (len_array) tecc_pfree(len_array);
		return -1;
	}

	field_start = pos;
	while (pos < end)
	{
		if (!in_quotes && *pos == delimiter)
		{
			/* End of field */
			int field_len = pos - field_start;

			/* Check if we need to expand arrays */
			if (field_idx >= max_fields)
			{
				max_fields *= 2;
				field_array = (char **)pg_realloc(field_array, max_fields * sizeof(char *));
				len_array = (int *)pg_realloc(len_array, max_fields * sizeof(int));
				if (field_array == NULL || len_array == NULL)
				{
					tecc_pfree(field_array);
					tecc_pfree(len_array);
					return -1;
				}
			}

			/* Allocate and copy field */
			field_array[field_idx] = (char *)pg_malloc(field_len + 1);
			if (field_array[field_idx] == NULL)
			{
				/* Cleanup allocated fields */
				for (int i = 0; i < field_idx; i++)
				{
					tecc_pfree(field_array[i]);
				}
				tecc_pfree(field_array);
				tecc_pfree(len_array);
				return -1;
			}
			memcpy(field_array[field_idx], field_start, field_len);
			field_array[field_idx][field_len] = '\0';
			len_array[field_idx] = field_len;
			field_idx++;

			/* Move to next field */
			pos++;
			field_start = pos;
		}
		else if (*pos == quote)
		{
			if (!in_quotes)
			{
				/* Start of quoted field */
				in_quotes = 1;
				field_start = pos + 1; /* Skip opening quote */
			}
			else if (pos + 1 < end && *(pos + 1) == quote)
			{
				/* Escaped quote inside quoted field */
				pos++; /* Skip the escaped quote */
			}
			else
			{
				/* End of quoted field */
				in_quotes = 0;
			}
			pos++;
		}
		else if (in_quotes && *pos == escape && escape != quote)
		{
			/* Escape character in quoted field */
			if (pos + 1 < end)
			{
				pos++; /* Skip escape character */
			}
			pos++; /* Skip escaped character */
		}
		else
		{
			/* Regular character */
			pos++;
		}
	}

	/* Process last field */
	if (pos >= field_start)
	{
		int field_len = pos - field_start;

		/* Check if we need to expand arrays */
		if (field_idx >= max_fields)
		{
			max_fields += 1;
			field_array = (char **)pg_realloc(field_array, max_fields * sizeof(char *));
			len_array = (int *)pg_realloc(len_array, max_fields * sizeof(int));
			if (field_array == NULL || len_array == NULL)
			{
				for (int i = 0; i < field_idx; i++)
				{
					tecc_pfree(field_array[i]);
				}
				tecc_pfree(field_array);
				tecc_pfree(len_array);
				return -1;
			}
		}

		/* Allocate and copy field */
		field_array[field_idx] = (char *)pg_malloc(field_len + 1);
		if (field_array[field_idx] == NULL)
		{
			/* Cleanup allocated fields */
			for (int i = 0; i < field_idx; i++)
			{
				tecc_pfree(field_array[i]);
			}
			tecc_pfree(field_array);
			tecc_pfree(len_array);
			return -1;
		}
		memcpy(field_array[field_idx], field_start, field_len);
		field_array[field_idx][field_len] = '\0';
		len_array[field_idx] = field_len;
		field_idx++;
	}

	/* ReqID:TECC4.0 -- Convert \N to NULL for consistency */
	{
		const char *null_str = "\\N";
		size_t null_len = 2;
		int i;
		for (i = 0; i < field_idx; i++)
		{
			if (len_array[i] == (int)null_len &&
				strncmp(field_array[i], null_str, null_len) == 0)
			{
				tecc_pfree(field_array[i]);
				field_array[i] = NULL;
				len_array[i] = 0;
			}
		}
	}
	/* Return results */
	*fields = field_array;
	*field_lens = len_array;
	*field_count = field_idx;

	return 0;
}

/*
 * tecc_copy_parse_text_row - Parse a TEXT format row into fields
 *
 * This function parses a TEXT format row according to PostgreSQL COPY TEXT rules.
 *
 * Parameters:
 *   row_data - Input row data
 *   row_len - Length of row data
 *   delimiter - Field delimiter (default '\t')
 *   fields - Output array for field pointers (allocated by this function)
 *   field_lens - Output array for field lengths (allocated by this function)
 *   field_count - Output number of fields found
 *
 * Returns:
 *   0 - Success
 *   -1 - Error
 */
static int
tecc_copy_parse_text_row(const char *row_data, int row_len,
                         char delimiter,
                         char ***fields, int **field_lens, int *field_count)
{
	const char *pos = row_data;
	const char *end = row_data + row_len;
	char **field_array = NULL;
	int *len_array = NULL;
	int max_fields = 10;
	int field_idx = 0;
	const char *field_start = NULL;

	if (row_data == NULL || row_len <= 0 || fields == NULL ||
	    field_lens == NULL || field_count == NULL)
	{
		return -1;
	}

	/* Allocate initial arrays */
	field_array = (char **)pg_malloc(max_fields * sizeof(char *));
	len_array = (int *)pg_malloc(max_fields * sizeof(int));
	if (field_array == NULL || len_array == NULL)
	{
		if (field_array) tecc_pfree(field_array);
		if (len_array) tecc_pfree(len_array);
		return -1;
	}

	field_start = pos;
	while (pos < end)
	{
		if (*pos == delimiter)
		{
			/* End of field */
			int field_len = pos - field_start;

			/* Check if we need to expand arrays */
			if (field_idx >= max_fields)
			{
				max_fields *= 2;
				field_array = (char **)pg_realloc(field_array, max_fields * sizeof(char *));
				len_array = (int *)pg_realloc(len_array, max_fields * sizeof(int));
				if (field_array == NULL || len_array == NULL)
				{
					tecc_pfree(field_array);
					tecc_pfree(len_array);
					return -1;
				}
			}

			/* Allocate and copy field */
			field_array[field_idx] = (char *)pg_malloc(field_len + 1);
			if (field_array[field_idx] == NULL)
			{
				/* Cleanup allocated fields */
				for (int i = 0; i < field_idx; i++)
				{
					tecc_pfree(field_array[i]);
				}
				tecc_pfree(field_array);
				tecc_pfree(len_array);
				return -1;
			}
			memcpy(field_array[field_idx], field_start, field_len);
			field_array[field_idx][field_len] = '\0';
			len_array[field_idx] = field_len;
			field_idx++;

			/* Move to next field */
			pos++;
			field_start = pos;
		}
		else if (*pos == '\\')
		{
			/* Escape sequence in TEXT mode */
			if (pos + 1 < end)
			{
				char next_char = *(pos + 1);
				if (next_char == '\\' || next_char == delimiter || next_char == '\n' || next_char == '\r' || next_char == '\t')
				{
					/* Valid escape sequence, skip the backslash */
					pos++;
				}
			}
			pos++;
		}
		else
		{
			/* Regular character */
			pos++;
		}
	}

	/* Process last field */
	if (pos >= field_start)
	{
		int field_len = pos - field_start;

		/* Check if we need to expand arrays */
		if (field_idx >= max_fields)
		{
			max_fields += 1;
			field_array = (char **)pg_realloc(field_array, max_fields * sizeof(char *));
			len_array = (int *)pg_realloc(len_array, max_fields * sizeof(int));
			if (field_array == NULL || len_array == NULL)
			{
				for (int i = 0; i < field_idx; i++)
				{
					tecc_pfree(field_array[i]);
				}
				tecc_pfree(field_array);
				tecc_pfree(len_array);
				return -1;
			}
		}

		/* Allocate and copy field */
		field_array[field_idx] = (char *)pg_malloc(field_len + 1);
		if (field_array[field_idx] == NULL)
		{
			/* Cleanup allocated fields */
			for (int i = 0; i < field_idx; i++)
			{
				tecc_pfree(field_array[i]);
			}
			tecc_pfree(field_array);
			tecc_pfree(len_array);
			return -1;
		}
		memcpy(field_array[field_idx], field_start, field_len);
		field_array[field_idx][field_len] = '\0';
		len_array[field_idx] = field_len;
		field_idx++;
	}

	/* ReqID:TECC4.0 — Convert \N to NULL for consistency */
	{
		const char *null_str = "\\N";
		size_t null_len = 2;
		int i;
		for (i = 0; i < field_idx; i++)
		{
			if (len_array[i] == (int)null_len &&
				strncmp(field_array[i], null_str, null_len) == 0)
			{
				tecc_pfree(field_array[i]);
				field_array[i] = NULL;
				len_array[i] = 0;
			}
		}
	}

	/* Return results */
	*fields = field_array;
	*field_lens = len_array;
	*field_count = field_idx;

	return 0;
}

/*
 * tecc_copy_format_csv_row - Format fields into a CSV row
 *
 * This function formats fields into a CSV format row according to PostgreSQL COPY CSV rules.
 *
 * Parameters:
 *   fields - Array of field strings
 *   field_lens - Array of field lengths
 *   field_count - Number of fields
 *   delimiter - Field delimiter (default ',')
 *   quote - Quote character (default '"')
 *   escape - Escape character (default '"' for CSV mode)
 *   formatted_row - Output formatted row (allocated by this function)
 *   formatted_len - Output formatted row length
 *
 * Returns:
 *   0 - Success
 *   -1 - Error
 */
static int
tecc_copy_format_csv_row(char **fields, int *field_lens, int field_count,
                         char delimiter, char quote, char escape,
                         char **formatted_row, int *formatted_len)
{
	int total_len = 0;
	char *result = NULL;
	int pos = 0;
	int i;

	if (fields == NULL || field_lens == NULL || field_count <= 0 ||
	    formatted_row == NULL || formatted_len == NULL)
	{
		return -1;
	}

	/* Calculate total length needed */
	for (i = 0; i < field_count; i++)
	{
		const char *field = fields[i];
		int field_len = field_lens[i];
		int needs_quotes = 0;
		int j;

		/* Check if field needs quotes */
		if (field == NULL)
		{
			/* NULL field */
			total_len += 2; /* Two quotes */
			continue;
		}

		for (j = 0; j < field_len; j++)
		{
			char c = field[j];
			if (c == delimiter || c == quote || c == '\n' || c == '\r')
			{
				needs_quotes = 1;
				break;
			}
		}

		if (needs_quotes)
		{
			/* Field needs quotes */
			total_len += 2; /* Opening and closing quotes */
			for (j = 0; j < field_len; j++)
			{
				if (field[j] == quote)
				{
					total_len++; /* Extra quote for escaping */
				}
			}
			total_len += field_len;
		}
		else
		{
			/* Field doesn't need quotes */
			total_len += field_len;
		}

		/* Add delimiter (except for last field) */
		if (i < field_count - 1)
		{
			total_len++;
		}
	}

	/* Allocate result buffer */
	result = (char *)pg_malloc(total_len + 1); /* +1 for null terminator */
	if (result == NULL)
	{
		return -1;
	}

	/* Format fields */
	for (i = 0; i < field_count; i++)
	{
		const char *field = fields[i];
		int field_len = field_lens[i];
		int needs_quotes = 0;
		int j;

		if (field == NULL)
		{
			/* NULL field - output as empty quoted string */
			result[pos++] = quote;
			result[pos++] = quote;
		}
		else
		{
			/* Check if field needs quotes */
			for (j = 0; j < field_len; j++)
			{
				char c = field[j];
				if (c == delimiter || c == quote || c == '\n' || c == '\r')
				{
					needs_quotes = 1;
					break;
				}
			}

			if (needs_quotes)
			{
				/* Field needs quotes */
				result[pos++] = quote;
				for (j = 0; j < field_len; j++)
				{
					if (field[j] == quote)
					{
						result[pos++] = quote; /* Escape quote by doubling */
					}
					result[pos++] = field[j];
				}
				result[pos++] = quote;
			}
			else
			{
				/* Field doesn't need quotes */
				memcpy(result + pos, field, field_len);
				pos += field_len;
			}
		}

		/* Add delimiter (except for last field) */
		if (i < field_count - 1)
		{
			result[pos++] = delimiter;
		}
	}

	/* Set output parameters */
	result[pos] = '\0';
	*formatted_row = result;
	*formatted_len = pos;

	return 0;
}

/*
 * tecc_copy_format_text_row - Format fields into a TEXT row
 *
 * This function formats fields into a TEXT format row according to PostgreSQL COPY TEXT rules.
 *
 * Parameters:
 *   fields - Array of field strings
 *   field_lens - Array of field lengths
 *   field_count - Number of fields
 *   delimiter - Field delimiter (default '\t')
 *   formatted_row - Output formatted row (allocated by this function)
 *   formatted_len - Output formatted row length
 *
 * Returns:
 *   0 - Success
 *   -1 - Error
 */
static int
tecc_copy_format_text_row(char **fields, int *field_lens, int field_count,
                          char delimiter,
                          char **formatted_row, int *formatted_len)
{
	int total_len = 0;
	char *result = NULL;
	int pos = 0;
	int i;

	if (fields == NULL || field_lens == NULL || field_count <= 0 ||
	    formatted_row == NULL || formatted_len == NULL)
	{
		return -1;
	}

	/* Calculate total length needed */
	for (i = 0; i < field_count; i++)
	{
		const char *field = fields[i];
		int field_len = field_lens[i];
		int j;

		if (field == NULL)
		{
			/* NULL field - output as \N */
			total_len += 2; /* \N */
			continue;
		}

		/* Calculate escaped length */
		for (j = 0; j < field_len; j++)
		{
			char c = field[j];
			if (c == '\\' || c == delimiter || c == '\n' || c == '\r' || c == '\t')
			{
				total_len++; /* Extra backslash for escaping */
			}
			total_len++;
		}

		/* Add delimiter (except for last field) */
		if (i < field_count - 1)
		{
			total_len++;
		}
	}

	/* Allocate result buffer */
	result = (char *)pg_malloc(total_len + 1); /* +1 for null terminator */
	if (result == NULL)
	{
		return -1;
	}

	/* Format fields */
	for (i = 0; i < field_count; i++)
	{
		const char *field = fields[i];
		int field_len = field_lens[i];
		int j;

		if (field == NULL)
		{
			/* NULL field - output as \N */
			result[pos++] = '\\';
			result[pos++] = 'N';
		}
		else
		{
			/* Escape special characters */
			for (j = 0; j < field_len; j++)
			{
				char c = field[j];
				if (c == '\\' || c == delimiter || c == '\n' || c == '\r' || c == '\t')
				{
					result[pos++] = '\\';
				}
				result[pos++] = c;
			}
		}

		/* Add delimiter (except for last field) */
		if (i < field_count - 1)
		{
			result[pos++] = delimiter;
		}
	}

	/* Set output parameters */
	result[pos] = '\0';
	*formatted_row = result;
	*formatted_len = pos;

	return 0;
}

/*
 * tecc_copy_free_parsed_fields - Free memory allocated by parse functions
 *
 * Parameters:
 *   fields - Array of field strings
 *   field_lens - Array of field lengths
 *   field_count - Number of fields
 */
static void
tecc_copy_free_parsed_fields(char **fields, int *field_lens, int field_count)
{
	int i;

	if (fields != NULL)
	{
		for (i = 0; i < field_count; i++)
		{
			if (fields[i] != NULL)
			{
				tecc_pfree(fields[i]);
			}
		}
		tecc_pfree(fields);
	}

	if (field_lens != NULL)
	{
		tecc_pfree(field_lens);
	}
}

/* ==================== Batch Processing Functions ==================== */

/* Begin - ReqID:TECC4.0 - Batch processing functions for import/export module */

/*
 * tecc_copy_enable_batch_processing - Enable batch processing for COPY operations
 *
 * Parameters:
 *   conn - Database connection
 *   batch_size - Batch size (number of rows per batch)
 *
 * Returns: 0 on success, -1 on failure
 */
int
tecc_copy_enable_batch_processing(PGconn *conn, int batch_size)
{
	TeccState *state;
	TeccEnhancedCopyState *copy_state;

	/* Validate parameters */
	if (conn == NULL || batch_size <= 0)
	{
		return -1;
	}

	/* Get TECC state */
	state = tecc_get_state(conn);
	if (state == NULL || !state->enabled)
	{
		/* TECC not enabled */
		return -1;
	}

	/* Get COPY state */
	copy_state = (TeccEnhancedCopyState *)conn->tecc_copy_state;
	if (copy_state == NULL)
	{
		tecc_processor_debug(TECC_LOG_DEBUG1,
			"tecc_preprocess_copy: COPY state creation skipped (server-side file COPY or parse error)");
		/*
		 * Don't cleanup existing state on failure — it may have been set
		 * by a prior call (e.g., copy.c:471 before SendQuery/PQexec).
		 * Keeping it allows PQputCopyData encryption to still work.
		 */
		return NULL;
	}

	/* Enable batch processing */
	copy_state->batch_processing = true;
	copy_state->batch_size = batch_size;

	/* Set default max batch size if not set */
	if (copy_state->max_batch_size == 0)
	{
		copy_state->max_batch_size = batch_size * 10; /* 10x batch size */
	}

	/* Initialize batch statistics */
	copy_state->progress.total_rows = 0;
	copy_state->progress.processed_rows = 0;

	/* Initialize error handling */
	if (copy_state->max_errors == 0)
	{
		copy_state->max_errors = 100; /* Default max errors */
	}

	tecc_record_debug(tecc_get_state(conn), "Batch processing enabled with batch size %d", batch_size);
	return 0;
}

/*
 * tecc_copy_disable_batch_processing - Disable batch processing for COPY operations
 *
 * Parameters:
 *   conn - Database connection
 *
 * Returns: 0 on success, -1 on failure
 */
int
tecc_copy_disable_batch_processing(PGconn *conn)
{
	TeccState *state;
	TeccEnhancedCopyState *copy_state;

	/* Validate parameters */
	if (conn == NULL)
	{
		return -1;
	}

	/* Get TECC state */
	state = tecc_get_state(conn);
	if (state == NULL || !state->enabled)
	{
		/* TECC not enabled */
		return -1;
	}

	/* Get COPY state */
	copy_state = (TeccEnhancedCopyState *)conn->tecc_copy_state;
	if (copy_state == NULL)
	{
		tecc_processor_debug(TECC_LOG_DEBUG1,
			"tecc_preprocess_copy: COPY state creation skipped (server-side file COPY or parse error)");
		/*
		 * Don't cleanup existing state on failure — it may have been set
		 * by a prior call (e.g., copy.c:471 before SendQuery/PQexec).
		 * Keeping it allows PQputCopyData encryption to still work.
		 */
		return NULL;
	}

	/* Disable batch processing */
	copy_state->batch_processing = false;

	tecc_record_debug(tecc_get_state(conn), "Batch processing disabled");
	return 0;
}

/*
 * tecc_copy_set_batch_size - Set batch size for batch processing
 *
 * Parameters:
 *   conn - Database connection
 *   batch_size - Batch size (number of rows per batch)
 *
 * Returns: 0 on success, -1 on failure
 */
int
tecc_copy_set_batch_size(PGconn *conn, int batch_size)
{
	TeccState *state;
	TeccEnhancedCopyState *copy_state;

	/* Validate parameters */
	if (conn == NULL || batch_size <= 0)
	{
		return -1;
	}

	/* Get TECC state */
	state = tecc_get_state(conn);
	if (state == NULL || !state->enabled)
	{
		/* TECC not enabled */
		return -1;
	}

	/* Get COPY state */
	copy_state = (TeccEnhancedCopyState *)conn->tecc_copy_state;
	if (copy_state == NULL)
	{
		tecc_processor_debug(TECC_LOG_DEBUG1,
			"tecc_preprocess_copy: COPY state creation skipped (server-side file COPY or parse error)");
		/*
		 * Don't cleanup existing state on failure — it may have been set
		 * by a prior call (e.g., copy.c:471 before SendQuery/PQexec).
		 * Keeping it allows PQputCopyData encryption to still work.
		 */
		return NULL;
	}

	/* Set batch size */
	copy_state->batch_size = batch_size;

	tecc_record_debug(tecc_get_state(conn), "Batch size set to %d", batch_size);
	return 0;
}

/*
 * tecc_copy_set_max_errors - Set maximum number of errors allowed in batch processing
 *
 * Parameters:
 *   conn - Database connection
 *   max_errors - Maximum number of errors allowed
 *
 * Returns: 0 on success, -1 on failure
 */
int
tecc_copy_set_max_errors(PGconn *conn, int max_errors)
{
	TeccState *state;
	TeccEnhancedCopyState *copy_state;

	/* Validate parameters */
	if (conn == NULL || max_errors < 0)
	{
		return -1;
	}

	/* Get TECC state */
	state = tecc_get_state(conn);
	if (state == NULL || !state->enabled)
	{
		/* TECC not enabled */
		return -1;
	}

	/* Get COPY state */
	copy_state = (TeccEnhancedCopyState *)conn->tecc_copy_state;
	if (copy_state == NULL)
	{
		tecc_processor_debug(TECC_LOG_DEBUG1,
			"tecc_preprocess_copy: COPY state creation skipped (server-side file COPY or parse error)");
		/*
		 * Don't cleanup existing state on failure — it may have been set
		 * by a prior call (e.g., copy.c:471 before SendQuery/PQexec).
		 * Keeping it allows PQputCopyData encryption to still work.
		 */
		return NULL;
	}

	/* Set max errors */
	copy_state->max_errors = max_errors;

	tecc_record_debug(tecc_get_state(conn), "Maximum errors set to %d", max_errors);
	return 0;
}

/*
 * tecc_copy_set_stop_on_error - Set whether to stop processing on error
 *
 * Parameters:
 *   conn - Database connection
 *   stop_on_error - True to stop on error, false to continue
 *
 * Returns: 0 on success, -1 on failure
 */
int
tecc_copy_set_stop_on_error(PGconn *conn, bool stop_on_error)
{
	TeccState *state;
	TeccEnhancedCopyState *copy_state;

	/* Validate parameters */
	if (conn == NULL)
	{
		return -1;
	}

	/* Get TECC state */
	state = tecc_get_state(conn);
	if (state == NULL || !state->enabled)
	{
		/* TECC not enabled */
		return -1;
	}

	/* Get COPY state */
	copy_state = (TeccEnhancedCopyState *)conn->tecc_copy_state;
	if (copy_state == NULL)
	{
		tecc_processor_debug(TECC_LOG_DEBUG1,
			"tecc_preprocess_copy: COPY state creation skipped (server-side file COPY or parse error)");
		/*
		 * Don't cleanup existing state on failure — it may have been set
		 * by a prior call (e.g., copy.c:471 before SendQuery/PQexec).
		 * Keeping it allows PQputCopyData encryption to still work.
		 */
		return NULL;
	}

	/* Set stop on error flag */
	copy_state->stop_on_error = stop_on_error;

	tecc_record_debug(tecc_get_state(conn), "Stop on error set to %s", stop_on_error ? "true" : "false");
	return 0;
}


/*
 * tecc_process_copy_data_batch - Process COPY data in batch mode for import
 *
 * This function processes COPY data in batch mode, accumulating rows until
 * batch size is reached, then processing the entire batch at once.
 *
 * Parameters:
 *   conn - Database connection
 *   data - Input data buffer
 *   data_len - Input data length
 *   processed_data - Output processed data buffer
 *   processed_len - Output processed data length
 *   rows_processed - Output number of rows processed
 *   errors_found - Output number of errors found
 *
 * Returns: 0 on success, -1 on failure
 */
int
tecc_process_copy_data_batch(PGconn *conn, const char *data, int data_len,
                             char **processed_data, int *processed_len,
                             int *rows_processed, int *errors_found)
{
	TeccState *state;
	TeccEnhancedCopyState *copy_state;
	char *result_buffer = NULL;
	int result_buffer_size = 0;
	int result_buffer_pos = 0;
	const char *current_pos = data;
	const char *end_pos = data + data_len;
	int local_rows_processed = 0;
	int local_errors_found = 0;
	bool batch_complete = false;

	/* Enhanced input validation */
	if (conn == NULL || data == NULL || processed_data == NULL ||
	    processed_len == NULL || rows_processed == NULL || errors_found == NULL)
	{
		return -1;
	}

	/* Validate data length */
	if (data_len < 0)
	{
		return -1;
	}

	/* Initialize output parameters */
	*rows_processed = 0;
	*errors_found = 0;

	/* Check for empty data */
	if (data_len == 0)
	{
		*processed_data = (char *)data;
		*processed_len = 0;
		return 0;
	}

	/* Get TECC state */
	state = tecc_get_state(conn);
	if (state == NULL || !state->enabled)
	{
		/* TECC not enabled, return original data */
		*processed_data = (char *)data;
		*processed_len = data_len;
		return 0;
	}

	/* Get COPY state */
	copy_state = (TeccEnhancedCopyState *)conn->tecc_copy_state;
	if (copy_state == NULL || !copy_state->is_from_stdin)
	{
		/* Not in COPY FROM STDIN mode, return original data */
		*processed_data = (char *)data;
		*processed_len = data_len;
		return 0;
	}

	/* Check if batch processing is enabled */
	if (!copy_state->batch_processing)
	{
		/* Batch processing not enabled, fall back to regular processing */
		return tecc_process_copy_data(conn, data, data_len,
		                              processed_data, processed_len);
	}

	/* Check if there are encrypted columns to process */
	if (copy_state->encrypted_column_count == 0)
	{
		/* No encrypted columns, return original data */
		*processed_data = (char *)data;
		*processed_len = data_len;
		return 0;
	}

	/* Initialize result buffer */
	result_buffer_size = data_len * 2; /* Allocate extra space */
	result_buffer = (char *)pg_malloc(result_buffer_size);
	if (result_buffer == NULL)
	{
		tecc_handle_error(conn, TECC_ERR_MEMORY,
		                  "Failed to allocate memory for batch COPY data processing");
		return -1;
	}

	/* Process data line by line */
	while (current_pos < end_pos && !batch_complete)
	{
		const char *line_start = current_pos;
		const char *line_end = current_pos;
		int line_len = 0;
		char *processed_row = NULL;
		int processed_row_len = 0;
		int row_result;

		/* Find end of line (newline or end of data) */
		while (line_end < end_pos && *line_end != '\n')
		{
			line_end++;
		}

		line_len = line_end - line_start;

		/* Process the line if it's not empty */
		if (line_len > 0)
		{
			/* Process the row */
			row_result = tecc_process_copy_row(copy_state, line_start, line_len,
			                                   &processed_row, &processed_row_len, true);

			if (row_result == 0 && processed_row != NULL && processed_row_len > 0)
			{
				/* Check if we have enough space in result buffer */
				size_t needed_space = result_buffer_pos + processed_row_len + 1; /* +1 for newline */
				if (needed_space > result_buffer_size)
				{
					/* Resize buffer */
					size_t new_size = result_buffer_size * 2;
					char *new_buffer = (char *)pg_realloc(result_buffer, new_size);
					if (new_buffer == NULL)
					{
						tecc_handle_error(conn, TECC_ERR_MEMORY,
						                  "Failed to resize buffer for batch COPY data");
						if (processed_row != line_start) /* Don't free original buffer */
						{
							tecc_pfree(processed_row);
						}
						tecc_pfree(result_buffer);
						return -1;
					}
					result_buffer = new_buffer;
					result_buffer_size = new_size;
				}

				/* Copy processed row to result buffer */
				memcpy(result_buffer + result_buffer_pos, processed_row, processed_row_len);
				result_buffer_pos += processed_row_len;

				/* Add newline if not at end of data */
				if (line_end < end_pos)
				{
					result_buffer[result_buffer_pos++] = '\n';
				}

				local_rows_processed++;

				/* Free processed row if it was allocated */
				if (processed_row != line_start) /* Don't free original buffer */
				{
					tecc_pfree(processed_row);
				}
			}
			else
			{
				/* Row processing failed */
				local_errors_found++;

				/* Add error to error list */
				tecc_copy_state_add_error(copy_state,
				                          TECC_COPY_ENCRYPTION_ERROR,
				                          "Failed to process row in batch mode",
				                          copy_state->current_row + local_rows_processed,
				                          -1);

				/* Check if we should stop on error */
				if (copy_state->stop_on_error)
				{
					char error_msg[256];
					snprintf(error_msg, sizeof(error_msg),
					         "Stopping batch processing due to error at row %d",
					         copy_state->current_row + local_rows_processed);
					tecc_handle_error(conn, TECC_COPY_ENCRYPTION_ERROR, error_msg);
					batch_complete = true;
				}
			}
		}

		/* Move to next line */
		if (line_end < end_pos && *line_end == '\n')
		{
			line_end++; /* Skip newline character */
		}
		current_pos = line_end;

		/* Check if batch is complete */
		if (local_rows_processed >= copy_state->batch_size)
		{
			batch_complete = true;
		}
	}

	/* Update COPY state */
	copy_state->current_row += local_rows_processed;

	/* Set output parameters */
	if (result_buffer_pos > 0)
	{
		*processed_data = result_buffer;
		*processed_len = result_buffer_pos;
	}
	else
	{
		/* No data processed */
		tecc_pfree(result_buffer);
		*processed_data = (char *)data;
		*processed_len = 0;
	}

	*rows_processed = local_rows_processed;
	*errors_found = local_errors_found;

	/* Update progress */
	copy_state->progress.processed_rows += local_rows_processed;

	tecc_record_debug(tecc_get_state(conn),
	                  "Batch processing completed: %d rows processed, %d errors found",
	                  local_rows_processed, local_errors_found);

	return (local_errors_found > 0 && copy_state->stop_on_error) ? -1 : 0;
}

/*
 * tecc_process_copy_result_batch - Process COPY result in batch mode for export
 *
 * This function processes COPY result data in batch mode for export operations.
 *
 * Parameters:
 *   conn - Database connection
 *   encrypted_data - Input encrypted data buffer
 *   encrypted_len - Input encrypted data length
 *   decrypted_data - Output decrypted data buffer
 *   decrypted_len - Output decrypted data length
 *   rows_processed - Output number of rows processed
 *   errors_found - Output number of errors found
 *
 * Returns: 0 on success, -1 on failure
 */
int
tecc_process_copy_result_batch(PGconn *conn, const char *encrypted_data,
                               int encrypted_len, char **decrypted_data,
                               int *decrypted_len, int *rows_processed,
                               int *errors_found)
{
	TeccState *state;
	TeccEnhancedCopyState *copy_state;
	char *result_buffer = NULL;
	int result_buffer_size = 0;
	int result_buffer_pos = 0;
	const char *current_pos = encrypted_data;
	const char *end_pos = encrypted_data + encrypted_len;
	int local_rows_processed = 0;
	int local_errors_found = 0;
	bool batch_complete = false;

	/* Enhanced input validation */
	if (conn == NULL || encrypted_data == NULL || decrypted_data == NULL ||
	    decrypted_len == NULL || rows_processed == NULL || errors_found == NULL)
	{
		return -1;
	}

	/* Validate data length */
	if (encrypted_len < 0)
	{
		return -1;
	}

	/* Initialize output parameters */
	*rows_processed = 0;
	*errors_found = 0;

	/* Check for empty data */
	if (encrypted_len == 0)
	{
		*decrypted_data = (char *)encrypted_data;
		*decrypted_len = 0;
		return 0;
	}

	/* Get TECC state */
	state = tecc_get_state(conn);
	if (state == NULL || !state->enabled)
	{
		/* TECC not enabled, return original data */
		*decrypted_data = (char *)encrypted_data;
		*decrypted_len = encrypted_len;
		return 0;
	}

	/* Get COPY state */
	copy_state = (TeccEnhancedCopyState *)conn->tecc_copy_state;
	if (copy_state == NULL || !copy_state->is_to_stdout)
	{
		/* Not in COPY TO STDOUT mode, return original data */
		*decrypted_data = (char *)encrypted_data;
		*decrypted_len = encrypted_len;
		return 0;
	}

	/* Check if batch processing is enabled */
	if (!copy_state->batch_processing)
	{
		/* Batch processing not enabled, fall back to regular processing */
		return tecc_process_copy_result(conn, encrypted_data, encrypted_len,
		                                decrypted_data, decrypted_len);
	}

	/* Check if there are encrypted columns to process */
	if (copy_state->encrypted_column_count == 0)
	{
		/* No encrypted columns, return original data */
		*decrypted_data = (char *)encrypted_data;
		*decrypted_len = encrypted_len;
		return 0;
	}

	/* Initialize result buffer */
	result_buffer_size = encrypted_len * 2; /* Allocate extra space */
	result_buffer = (char *)pg_malloc(result_buffer_size);
	if (result_buffer == NULL)
	{
		tecc_handle_error(conn, TECC_ERR_MEMORY,
		                  "Failed to allocate memory for batch COPY result processing");
		return -1;
	}

	/* Process data line by line */
	while (current_pos < end_pos && !batch_complete)
	{
		const char *line_start = current_pos;
		const char *line_end = current_pos;
		int line_len = 0;
		char *processed_row = NULL;
		int processed_row_len = 0;
		int row_result;

		/* Find end of line (newline or end of data) */
		while (line_end < end_pos && *line_end != '\n')
		{
			line_end++;
		}

		line_len = line_end - line_start;

		/* Process the line if it's not empty */
		if (line_len > 0)
		{
			/* Process the row (decrypt for export) */
			row_result = tecc_process_copy_row(copy_state, line_start, line_len,
			                                   &processed_row, &processed_row_len, false);

			if (row_result == 0 && processed_row != NULL && processed_row_len > 0)
			{
				/* Check if we have enough space in result buffer */
				size_t needed_space = result_buffer_pos + processed_row_len + 1; /* +1 for newline */
				if (needed_space > result_buffer_size)
				{
					/* Resize buffer */
					size_t new_size = result_buffer_size * 2;
					char *new_buffer = (char *)pg_realloc(result_buffer, new_size);
					if (new_buffer == NULL)
					{
						tecc_handle_error(conn, TECC_ERR_MEMORY,
						                  "Failed to resize buffer for batch COPY result");
						if (processed_row != line_start) /* Don't free original buffer */
						{
							tecc_pfree(processed_row);
						}
						tecc_pfree(result_buffer);
						return -1;
					}
					result_buffer = new_buffer;
					result_buffer_size = new_size;
				}

				/* Copy processed row to result buffer */
				memcpy(result_buffer + result_buffer_pos, processed_row, processed_row_len);
				result_buffer_pos += processed_row_len;

				/* Add newline if not at end of data */
				if (line_end < end_pos)
				{
					result_buffer[result_buffer_pos++] = '\n';
				}

				local_rows_processed++;

				/* Free processed row if it was allocated */
				if (processed_row != line_start) /* Don't free original buffer */
				{
					tecc_pfree(processed_row);
				}
			}
			else
			{
				/* Row processing failed */
				local_errors_found++;

				/* Add error to error list */
				tecc_copy_state_add_error(copy_state,
				                          TECC_COPY_DECRYPTION_ERROR,
				                          "Failed to decrypt row in batch mode",
				                          copy_state->current_row + local_rows_processed,
				                          -1);

				/* Check if we should stop on error */
				if (copy_state->stop_on_error)
				{
					char error_msg[256];
					snprintf(error_msg, sizeof(error_msg),
					         "Stopping batch processing due to error at row %d",
					         copy_state->current_row + local_rows_processed);
					tecc_handle_error(conn, TECC_COPY_DECRYPTION_ERROR, error_msg);
					batch_complete = true;
				}
			}
		}

		/* Move to next line */
		if (line_end < end_pos && *line_end == '\n')
		{
			line_end++; /* Skip newline character */
		}
		current_pos = line_end;

		/* Check if batch is complete */
		if (local_rows_processed >= copy_state->batch_size)
		{
			batch_complete = true;
		}
	}

	/* Update COPY state */
	copy_state->current_row += local_rows_processed;

	/* Set output parameters */
	if (result_buffer_pos > 0)
	{
		*decrypted_data = result_buffer;
		*decrypted_len = result_buffer_pos;
	}
	else
	{
		/* No data processed */
		tecc_pfree(result_buffer);
		*decrypted_data = (char *)encrypted_data;
		*decrypted_len = 0;
	}

	*rows_processed = local_rows_processed;
	*errors_found = local_errors_found;

	/* Update progress */
	copy_state->progress.processed_rows += local_rows_processed;

	tecc_record_debug(tecc_get_state(conn),
	                  "Batch result processing completed: %d rows processed, %d errors found",
	                  local_rows_processed, local_errors_found);

	return (local_errors_found > 0 && copy_state->stop_on_error) ? -1 : 0;
}

/*
 * tecc_copy_get_batch_statistics - Get batch processing statistics
 *
 * Parameters:
 *   conn - Database connection
 *   total_rows - Output total rows processed
 *   successful_rows - Output successful rows processed
 *   failed_rows - Output failed rows
 *   total_batches - Output total batches processed
 *   avg_batch_time - Output average batch processing time (seconds)
 *
 * Returns: 0 on success, -1 on failure
 */
int
tecc_copy_get_batch_statistics(PGconn *conn, int *total_rows,
                               int *successful_rows, int *failed_rows,
                               int *total_batches, double *avg_batch_time)
{
	TeccState *state;
	TeccEnhancedCopyState *copy_state;

	/* Validate parameters */
	if (conn == NULL || total_rows == NULL || successful_rows == NULL ||
	    failed_rows == NULL || total_batches == NULL || avg_batch_time == NULL)
	{
		return -1;
	}

	/* Get TECC state */
	state = tecc_get_state(conn);
	if (state == NULL || !state->enabled)
	{
		/* TECC not enabled */
		return -1;
	}

	/* Get COPY state */
	copy_state = (TeccEnhancedCopyState *)conn->tecc_copy_state;
	if (copy_state == NULL)
	{
		tecc_processor_debug(TECC_LOG_DEBUG1,
			"tecc_preprocess_copy: COPY state creation skipped (server-side file COPY or parse error)");
		/*
		 * Don't cleanup existing state on failure — it may have been set
		 * by a prior call (e.g., copy.c:471 before SendQuery/PQexec).
		 * Keeping it allows PQputCopyData encryption to still work.
		 */
		return NULL;
	}

	/* Calculate statistics */
	*total_rows = copy_state->progress.processed_rows;
	*successful_rows = copy_state->progress.processed_rows - copy_state->error_count;
	*failed_rows = copy_state->error_count;

	/* Calculate batches processed */
	if (copy_state->batch_size > 0)
	{
		*total_batches = (copy_state->progress.processed_rows + copy_state->batch_size - 1) /
		                 copy_state->batch_size;
	}
	else
	{
		*total_batches = 0;
	}

	/* Calculate average batch time */
	if (copy_state->progress.start_time.tv_sec > 0 && *total_batches > 0)
	{
		double total_time;
		struct timeval current_time;
		gettimeofday(&current_time, NULL);

		total_time = (current_time.tv_sec - copy_state->progress.start_time.tv_sec) +
		                   (current_time.tv_usec - copy_state->progress.start_time.tv_usec) / 1000000.0;

		*avg_batch_time = total_time / *total_batches;
	}
	else
	{
		*avg_batch_time = 0.0;
	}

	return 0;
}

/*
 * tecc_copy_get_batch_errors - Get batch processing errors
 *
 * Parameters:
 *   conn - Database connection
 *   errors - Output error array (caller must free)
 *   error_count - Output error count
 *
 * Returns: 0 on success, -1 on failure
 */
int
tecc_copy_get_batch_errors(PGconn *conn, TeccCopyError **errors,
                           int *error_count)
{
	TeccState *state;
	TeccEnhancedCopyState *copy_state;

	/* Validate parameters */
	if (conn == NULL || errors == NULL || error_count == NULL)
	{
		return -1;
	}

	/* Get TECC state */
	state = tecc_get_state(conn);
	if (state == NULL || !state->enabled)
	{
		/* TECC not enabled */
		return -1;
	}

	/* Get COPY state */
	copy_state = (TeccEnhancedCopyState *)conn->tecc_copy_state;
	if (copy_state == NULL)
	{
		tecc_processor_debug(TECC_LOG_DEBUG1,
			"tecc_preprocess_copy: COPY state creation skipped (server-side file COPY or parse error)");
		/*
		 * Don't cleanup existing state on failure — it may have been set
		 * by a prior call (e.g., copy.c:471 before SendQuery/PQexec).
		 * Keeping it allows PQputCopyData encryption to still work.
		 */
		return NULL;
	}

	/* Copy errors */
	*error_count = copy_state->error_count;
	if (copy_state->error_count > 0 && copy_state->errors != NULL)
	{
		size_t errors_size = sizeof(TeccCopyError) * copy_state->error_count;
		*errors = (TeccCopyError *)pg_malloc(errors_size);
		if (*errors == NULL)
		{
			return -1;
		}
		memcpy(*errors, copy_state->errors, errors_size);
	}
	else
	{
		*errors = NULL;
	}

	return 0;
}

/*
 * tecc_copy_reset_batch_statistics - Reset batch processing statistics
 *
 * Parameters:
 *   conn - Database connection
 *
 * Returns: 0 on success, -1 on failure
 */
int
tecc_copy_reset_batch_statistics(PGconn *conn)
{
	TeccState *state;
	TeccEnhancedCopyState *copy_state;

	/* Validate parameters */
	if (conn == NULL)
	{
		return -1;
	}

	/* Get TECC state */
	state = tecc_get_state(conn);
	if (state == NULL || !state->enabled)
	{
		/* TECC not enabled */
		return -1;
	}

	/* Get COPY state */
	copy_state = (TeccEnhancedCopyState *)conn->tecc_copy_state;
	if (copy_state == NULL)
	{
		tecc_processor_debug(TECC_LOG_DEBUG1,
			"tecc_preprocess_copy: COPY state creation skipped (server-side file COPY or parse error)");
		/*
		 * Don't cleanup existing state on failure — it may have been set
		 * by a prior call (e.g., copy.c:471 before SendQuery/PQexec).
		 * Keeping it allows PQputCopyData encryption to still work.
		 */
		return NULL;
	}

	/* Reset statistics */
	copy_state->progress.processed_rows = 0;
	copy_state->progress.processed_bytes = 0;
	gettimeofday(&copy_state->progress.start_time, NULL);

	/* Reset errors */
	if (copy_state->errors != NULL)
	{
		tecc_pfree(copy_state->errors);
		copy_state->errors = NULL;
	}
	copy_state->error_count = 0;

	tecc_record_debug(tecc_get_state(conn), "Batch statistics reset");
	return 0;
}

/* End - ReqID:TECC4.0 - Batch processing functions for import/export module */
