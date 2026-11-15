
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
#include "port/pg_bswap.h"
#include "fe-tecc-integration.h"
#include "fe-tecc-copy.h"

#include "../tecc_cache/tecc_metadata_cache.h"
#include "../tecc_cache/tecc_data_fetcher_adapter.h"
#include "../tecc_processor/tecc_processor.h"
#include "../tecc_processor/internal/tecc_processor_internal.h"
#include "../tecc_processor/processor_utils.h"
#include "../tecc_scheduler/tecc_crypto_scheduler.h"
#include "../tecc_scheduler/tecc_key_manager_adapter.h"
#include "../tecc_scheduler/tecc_algorithm_scheduler.h"
#include "../tecc_common/tecc_frontend_list.h"

#include "../frontend_parser/tecc_parser.h"
#include "../frontend_parser/tecc_nodes.h"
#include "../frontend_parser/tecc_error.h"
#include "../../../common/tecc_types.h"
#undef TECC_CLAMP
#include "../tecc_formatter/tecc_hex_encoder.h"
#include "../tecc_formatter/tecc_hex_encoder.h"

/* External declarations for utility functions from fe-tecc-integration.c */

/* PostgreSQL COPY binary format constants */
#define TECC_BINARY_SIGNATURE      "PGCOPY\n\377\r\n\0"
#define TECC_BINARY_SIG_LEN        11
#define TECC_BINARY_FLAGS_LEN      4
#define TECC_BINARY_EXT_LEN        4
#define TECC_BINARY_HEADER_LEN     (TECC_BINARY_SIG_LEN + TECC_BINARY_FLAGS_LEN + TECC_BINARY_EXT_LEN)

/* Static function prototypes */
static TeccCopyState *tecc_create_copy_state(PGconn *conn, const char *command);
static void tecc_free_copy_state(TeccCopyState *copy_state);
static int tecc_parse_copy_with_frontend_parser(PGconn *conn, const char *command, char **table_name,
                                           char ***column_names, int *column_count,
                                           int **column_mapping, int *mapping_count,
                                           char ***column_table_names,
                                           bool *is_from_stdin, bool *is_to_stdout,
                                           bool *csv_mode, bool *binary_mode,
                                           char *delimiter,
                                           char *quote, char *escape,
                                           bool *header, char **null_string,
                                           char **schema_name);
static int tecc_copy_state_load_encrypted_columns_for_table(TeccEnhancedCopyState *copy_state,
                                                             const char *table_name);
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
static int tecc_copy_parse_binary_row(const char *row_data, int row_len,
                                      char ***fields, int **field_lens, int *field_count);
static int tecc_copy_format_binary_row(char **fields, int *field_lens, int field_count,
                                       char **row_data, int *row_len);
/* ReqID:TECC4.0 - Convert binary field to TEXT representation by type */
static void
tecc_copy_binary_to_text(
	const unsigned char *data, int data_len,
	TeccEnhancedCopyState *copy_state, int field_idx,
	PQExpBuffer buf)
{
	int j;
	Oid base_oid = InvalidOid;
	/* ReqID:TECC4.0 - Bug 907: 多表 JOIN 时同 attnum 可能属于不同表，
	 * 需按本列来源表过滤，否则会拿错表的原始类型 OID 去转文本。 */
	const char *field_table = NULL;

	if (copy_state->column_table_names != NULL &&
		field_idx >= 0 && field_idx < copy_state->mapping_count)
		field_table = copy_state->column_table_names[field_idx];

	for (j = 0; j < copy_state->encrypted_column_count; j++)
	{
		TeccEncryptedColumnMetadata *col = copy_state->encrypted_column_info[j];
		if (col)
		{
			int att = copy_state->column_mapping ?
				copy_state->column_mapping[field_idx] : (field_idx + 1);
			if (col->column_attnum == att &&
				(field_table == NULL ||
				 (col->table_name != NULL &&
				  pg_strcasecmp(col->table_name, field_table) == 0)))
			{
				base_oid = col->data_type_original_oid;
				break;
			}
		}
	}
	if (base_oid == InvalidOid)
	{
		char *hex = NULL;
		if (tecc_hex_encoder_encode(data, (size_t)data_len, &hex) ==
			TECC_FORMATTER_SUCCESS && hex != NULL)
		{
			const char *p = hex;
			if (p[0] == '\\' && p[1] == 'x') p += 2;
			appendPQExpBufferStr(buf, p);
			tecc_pfree(hex);
		}
		return;
	}
	switch (base_oid)
	{
		case 23:
		{
			int32 val;
			char tmp[32];
			if (data_len >= 4)
			{
				memcpy(&val, data, 4);
				val = ntohl(val);
				snprintf(tmp, sizeof(tmp), "%d", val);
				appendPQExpBufferStr(buf, tmp);
			}
			return;
		}
		case 21:
		{
			int16 val;
			char tmp[32];
			if (data_len >= 2)
			{
				memcpy(&val, data, 2);
				val = ntohs(val);
				snprintf(tmp, sizeof(tmp), "%hd", val);
				appendPQExpBufferStr(buf, tmp);
			}
			return;
		}
		case 20:
		{
			uint64 uv = 0;
			char tmp[32];
			int _bi;
			for (_bi = 0; _bi < 8 && _bi < data_len; _bi++)
				uv = (uv << 8) | data[_bi];
			snprintf(tmp, sizeof(tmp), "%lld", (long long)uv);
			appendPQExpBufferStr(buf, tmp);
			return;
		}
		case 25:
		default:
		{
			int _ci;
			for (_ci = 0; _ci < data_len; _ci++)
			{
				char ch = data[_ci];
				if (ch == '\\' || ch == '\n' || ch == '\r')
					appendPQExpBufferChar(buf, '\\');
				appendPQExpBufferChar(buf, ch);
			}
			return;
		}
	}
}

/* Byte-swap 32-bit (network/host byte order conversion) */
static int32 tecc_copy_swap32(int32 v) {
	return (int32)(((uint32)v >> 24) | (((uint32)v >> 8) & 0xff00) |
	               (((uint32)v << 8) & 0xff0000) | ((uint32)v << 24));
}

/* Number of 32-bit words in the wire header for a packed version value.
 * Layout: version, dek_dist_oid, then one length word per present segment.
 * v1.0 keeps its historical fixed four-length layout; v2.0 is mask-driven,
 * so the count varies per value and must never be assumed constant. */
static int tecc_copy_header_words(int32 host_version)
{
	if (CIPHERDATA_MAJOR_VERSION(host_version) == CIPHERDATA_VERSION_1_0)
		return 2 + 4;
	return 2 + (int)CIPHERDATA_POPCOUNT16(CIPHERDATA_SEG_MASK(host_version));
}

/* Convert binary cipherdata from network byte order to host byte order
 * with varsize prepended, then hex-encode for TECC processing */
static int tecc_copy_cipherdata_to_hex(const unsigned char *bin, int bin_len,
                                        char **hex_out)
{
	int i, n32;
	int32 net_version;
	int32 host_version;
	int header_bytes;
	int32 total_sz;
	unsigned char *host_buf;
	int result;

	if (!bin || bin_len < (int)(2 * sizeof(int32)) || !hex_out) return -1;

	/* The on-wire version is network order; read it to learn how many
	 * length words follow so the rest of the header can be located. */
	memcpy(&net_version, bin, sizeof(int32));
	host_version = tecc_copy_swap32(net_version);
	n32 = tecc_copy_header_words(host_version);
	header_bytes = n32 * (int)sizeof(int32);
	if (bin_len < header_bytes) return -1;

	total_sz = (int32)(sizeof(int32) + header_bytes + (bin_len - header_bytes));
	host_buf = (unsigned char *)pg_malloc((size_t)total_sz);
	if (!host_buf) return -1;
	memcpy(host_buf, &total_sz, sizeof(int32));
	for (i = 0; i < n32; i++) {
		int32 net_val;
		memcpy(&net_val, bin + i * (int)sizeof(int32), sizeof(int32));
		((int32 *)(host_buf + sizeof(int32)))[i] = tecc_copy_swap32(net_val);
	}
	if (bin_len > header_bytes)
		memcpy(host_buf + sizeof(int32) + header_bytes,
		       bin + header_bytes,
		       (size_t)(bin_len - header_bytes));
	result = tecc_hex_encoder_encode(host_buf, (size_t)total_sz, hex_out);
	tecc_pfree(host_buf);
	return (result == TECC_FORMATTER_SUCCESS && *hex_out != NULL) ? 0 : -1;
}

/* Convert hex-encoded cipherdata back to network byte order
 * (strips varsize, byte-swaps header fields) for binary format */
static int tecc_copy_hex_to_cipherdata(const char *hex_str,
                                         unsigned char **bin_out, int *bin_len)
{
	unsigned char *raw_bytes = NULL;
	size_t raw_len = 0;
	int i, n32;
	int32 host_version;
	int header_bytes;
	int data_len;
	unsigned char *net_buf;
	int result;

	if (!hex_str || !bin_out || !bin_len) return -1;
	result = tecc_hex_encoder_decode(hex_str, &raw_bytes, &raw_len);
	if (result != TECC_FORMATTER_SUCCESS || !raw_bytes ||
		raw_len < sizeof(int32) + 2 * sizeof(int32)) {
		if (raw_bytes) tecc_pfree(raw_bytes);
		return -1;
	}

	/* raw_bytes is the host-order on-disk form: vl_len_, version, dek, ... */
	memcpy(&host_version, raw_bytes + sizeof(int32), sizeof(int32));
	n32 = tecc_copy_header_words(host_version);
	header_bytes = n32 * (int)sizeof(int32);
	if (raw_len < sizeof(int32) + (size_t)header_bytes) {
		tecc_pfree(raw_bytes);
		return -1;
	}

	data_len = (int)(raw_len - sizeof(int32));
	net_buf = (unsigned char *)pg_malloc((size_t)data_len);
	if (!net_buf) { tecc_pfree(raw_bytes); return -1; }
	for (i = 0; i < n32; i++) {
		int32 host_val = ((int32 *)(raw_bytes + sizeof(int32)))[i];
		int32 net_val = tecc_copy_swap32(host_val);
		memcpy(net_buf + i * (int)sizeof(int32), &net_val, sizeof(int32));
	}
	if (data_len > header_bytes)
		memcpy(net_buf + header_bytes,
		       raw_bytes + sizeof(int32) + header_bytes,
		       (size_t)(data_len - header_bytes));
	tecc_pfree(raw_bytes);
	*bin_out = net_buf;
	*bin_len = data_len;
	return 0;
}

/* ReqID:TECC4.0 - Check if field at given file-column index is an encrypted column */
static bool tecc_copy_is_field_encrypted(TeccEnhancedCopyState *copy_state, int field_idx)
{
	int j;
	int table_attnum;
	const char *field_table_name;
	const char *field_col_name;
	if (copy_state == NULL || copy_state->encrypted_column_count <= 0)
		return false;
	if (field_idx < 0)
		return false;
	if (copy_state->column_mapping != NULL &&
		copy_state->mapping_count > 0 &&
		field_idx < copy_state->mapping_count)
		table_attnum = copy_state->column_mapping[field_idx];
	else
		table_attnum = field_idx + 1;
	field_table_name = (copy_state->column_table_names != NULL &&
	                        field_idx < copy_state->mapping_count)
		? copy_state->column_table_names[field_idx] : NULL;
	field_col_name = (copy_state->column_names != NULL &&
	                      field_idx < copy_state->mapping_count)
		? copy_state->column_names[field_idx] : NULL;
	for (j = 0; j < copy_state->encrypted_column_count; j++)
	{
		TeccEncryptedColumnMetadata *col_info = copy_state->encrypted_column_info[j];
		if (col_info == NULL) continue;
		/* Multi-table: match by (table_name, column_name) */
		if (field_table_name != NULL && field_col_name != NULL &&
			col_info->table_name != NULL && col_info->column_name != NULL &&
			pg_strcasecmp(col_info->table_name, field_table_name) == 0 &&
			pg_strcasecmp(col_info->column_name, field_col_name) == 0)
			return true;
		/* Single-table fallback: match by (table_name, attnum) */
		if (col_info->column_attnum == table_attnum &&
			(field_table_name == NULL ||
			 (col_info->table_name != NULL &&
			  pg_strcasecmp(col_info->table_name, field_table_name) == 0)))
			return true;
	}
	return false;
}


/* Byte-swap 32-bit (network/host byte order conversion) */
static void tecc_copy_free_parsed_fields(char **fields, int *field_lens, int field_count);

/* COPY state helper functions (non-static, declared in tecc_common_types.h) */
int tecc_copy_state_load_encrypted_columns(TeccEnhancedCopyState *copy_state);
int tecc_copy_state_init_column_mapping(TeccEnhancedCopyState *copy_state);
int tecc_copy_state_init_crypto_context(TeccEnhancedCopyState *copy_state);
int tecc_copy_state_init_stream_contexts(TeccEnhancedCopyState *copy_state);
int tecc_copy_state_cleanup_stream_contexts(TeccEnhancedCopyState *copy_state);
void tecc_copy_state_cleanup_crypto_context(TeccEnhancedCopyState *copy_state);
/* ReqID:TECC4.0 - Bug 907：多表 JOIN 子查询的来源表元数据补载 */
int tecc_copy_state_load_mapped_tables(TeccEnhancedCopyState *copy_state);
int tecc_copy_state_reload_mapped_columns(TeccEnhancedCopyState *copy_state);
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

	/* Binary mode: process as binary COPY data (not line-based) */
	if (copy_state->binary_mode)
	{
		char *binary_result = NULL;
		int binary_result_size = 0;
		int binary_result_pos = 0;
		const char *bin_pos = data;
		const char *bin_end = data + data_len;

		/* Initialize result buffer */
		binary_result_size = data_len * 2 + 1024;
		binary_result = (char *)pg_malloc(binary_result_size);
		if (binary_result == NULL)
		{
			tecc_handle_error(conn, TECC_ERR_MEMORY,
			                  "Failed to allocate memory for binary COPY data");
			return -1;
		}

		/* On first call, skip the binary header (19 bytes) */
		if (copy_state->current_row == 0 && data_len >= TECC_BINARY_HEADER_LEN)
		{
			memcpy(binary_result, bin_pos, TECC_BINARY_HEADER_LEN);
			binary_result_pos = TECC_BINARY_HEADER_LEN;
			bin_pos += TECC_BINARY_HEADER_LEN;
		}

		/* Process remaining data as binary rows */
		while (bin_pos < bin_end)
		{
			int remaining = bin_end - bin_pos;
			uint16 fld_count_net;
			int16 fld_count;
			char **fields = NULL;
			int *field_lens = NULL;
			int field_count = 0;
			int parse_result;

			if (remaining < 2)
				break;

			memcpy(&fld_count_net, bin_pos, 2);
			fld_count = (int16) pg_ntoh16(fld_count_net);

			/* End-of-data marker */
			if (fld_count == -1)
			{
				if (binary_result_pos + 2 > binary_result_size)
				{
					int new_size = binary_result_size * 2;
					char *new_buf = (char *)pg_realloc(binary_result, new_size);
					if (new_buf == NULL) { tecc_pfree(binary_result); return -1; }
					binary_result = new_buf;
					binary_result_size = new_size;
				}
				memcpy(binary_result + binary_result_pos, bin_pos, 2);
				binary_result_pos += 2;
				bin_pos += 2;
				break;
			}

			/* Parse binary row into fields */
			parse_result = tecc_copy_parse_binary_row(bin_pos, remaining,
			                                         &fields, &field_lens,
			                                         &field_count);
			if (parse_result != 0 || fields == NULL)
			{
				tecc_pfree(binary_result);
				tecc_handle_error(conn, TECC_ERR_PROCESSOR,
				                  "Failed to parse binary COPY row");
				return -1;
			}

			/* Convert binary fields -> TEXT row -> processor -> TEXT -> binary */
			{
				PQExpBufferData text_row_buf;
				char *text_processed = NULL;
				int text_processed_len = 0;
				char **proc_fields = NULL;
				int *proc_field_lens = NULL;
				int proc_field_count = 0;
				char *binary_row = NULL;
				int binary_row_len = 0;
				int i, process_result, parse_proc_result;

				initPQExpBuffer(&text_row_buf);
				for (i = 0; i < field_count; i++)
				{
					if (i > 0)
						appendPQExpBufferChar(&text_row_buf, copy_state->delimiter);

					if (field_lens[i] == -1 || fields[i] == NULL)
						appendPQExpBufferStr(&text_row_buf, "\\N");
					else
					/* ReqID:TECC4.0 - Import: convert binary to TEXT by type */
					if (tecc_copy_is_field_encrypted(copy_state, i))
					{
						tecc_copy_binary_to_text(
							(const unsigned char *)fields[i], field_lens[i],
							copy_state, i,
							&text_row_buf);
					}
					else
					{
						/* Non-encrypted: hex-encode raw bytes (NO x prefix) */
						char *hex_out = NULL;
						if (tecc_hex_encoder_encode(
							(const unsigned char *)fields[i],
							(size_t)field_lens[i],
							&hex_out) == TECC_FORMATTER_SUCCESS && hex_out != NULL)
						{
							const char *hex_p = hex_out;
							if (hex_p[0] == '\\' && hex_p[1] == 'x')
								hex_p += 2;
							appendPQExpBufferStr(&text_row_buf, hex_p);
							tecc_pfree(hex_out);
						}
					}
				}

				/* Process TEXT row through standard processor (import/encrypt) */
				process_result = tecc_process_copy_row(copy_state,
				                                       text_row_buf.data,
				                                       text_row_buf.len,
				                                       &text_processed,
				                                       &text_processed_len,
				                                       true);
				if (process_result != 0)
				{
					termPQExpBuffer(&text_row_buf);
					tecc_copy_free_parsed_fields(fields, field_lens, field_count);
					tecc_pfree(binary_result);
					tecc_handle_error(conn, TECC_ERR_PROCESSOR,
					                  "Failed to process binary COPY row");
					return -1;
				}

				/* Parse processed TEXT row back into fields */
				parse_proc_result = tecc_copy_parse_text_row(text_processed,
				                                             text_processed_len,
				                                             copy_state->delimiter,
				                                             &proc_fields,
				                                             &proc_field_lens,
				                                             &proc_field_count);
				if (parse_proc_result != 0 || proc_fields == NULL)
				{
					termPQExpBuffer(&text_row_buf);
					tecc_copy_free_parsed_fields(fields, field_lens, field_count);
					tecc_pfree(binary_result);
					tecc_pfree(text_processed);
					tecc_handle_error(conn, TECC_ERR_PROCESSOR,
					                  "Failed to parse processed COPY row");
					return -1;
				}

				/* ReqID:TECC4.0 - Decode post-TECC fields to proper binary */
				{
					int _di;
					for (_di = 0; _di < proc_field_count; _di++)
					{
						if (proc_fields[_di] == NULL || proc_field_lens[_di] <= 0)
							continue;

						if (tecc_copy_is_field_encrypted(copy_state, _di))
						{
							/* Always hex decode via hex_to_cipherdata (processor strips \\x) */
							{
								unsigned char *raw_bytes = NULL;
								int raw_len = 0;
								if (tecc_copy_hex_to_cipherdata(
										proc_fields[_di],
										&raw_bytes, &raw_len) == 0 &&
								    raw_bytes != NULL && raw_len >= sizeof(int32) * 9)
								{
									tecc_pfree(proc_fields[_di]);
									proc_fields[_di] = (char *)raw_bytes;
									proc_field_lens[_di] = raw_len;
								}
							}
						}
						else
						{
							unsigned char *raw_bytes = NULL;
							size_t raw_len = 0;
							if (tecc_hex_encoder_decode(proc_fields[_di],
								&raw_bytes, &raw_len) == TECC_FORMATTER_SUCCESS &&
							    raw_bytes != NULL && raw_len > 0)
							{
								tecc_pfree(proc_fields[_di]);
								proc_fields[_di] = (char *)raw_bytes;
								proc_field_lens[_di] = (int)raw_len;
							}
						}
					}
				}
				/* Format processed fields back to binary */
				if (tecc_copy_format_binary_row(proc_fields, proc_field_lens,
				                                proc_field_count,
				                                &binary_row, &binary_row_len) != 0)
				{
					termPQExpBuffer(&text_row_buf);
					tecc_copy_free_parsed_fields(fields, field_lens, field_count);
					tecc_copy_free_parsed_fields(proc_fields, proc_field_lens,
					                             proc_field_count);
					tecc_pfree(binary_result);
					tecc_pfree(text_processed);
					tecc_handle_error(conn, TECC_ERR_PROCESSOR,
					                  "Failed to format binary row");
					return -1;
				}

				/* Append to result */
				while (binary_result_pos + binary_row_len > binary_result_size)
				{
					int new_size = binary_result_size * 2;
					char *new_buf = (char *)pg_realloc(binary_result, new_size);
					if (new_buf == NULL)
					{
						termPQExpBuffer(&text_row_buf);
						tecc_copy_free_parsed_fields(fields, field_lens, field_count);
						tecc_copy_free_parsed_fields(proc_fields, proc_field_lens,
						                             proc_field_count);
						tecc_pfree(binary_result);
						tecc_pfree(binary_row);
						tecc_pfree(text_processed);
						return -1;
					}
					binary_result = new_buf;
					binary_result_size = new_size;
				}
				memcpy(binary_result + binary_result_pos, binary_row,
				       binary_row_len);
				binary_result_pos += binary_row_len;

				tecc_pfree(binary_row);
				tecc_copy_free_parsed_fields(proc_fields, proc_field_lens,
				                             proc_field_count);
				termPQExpBuffer(&text_row_buf);
				tecc_pfree(text_processed);
			}

			/* Advance past the raw binary row */
			{
				int raw_row_len = 2;
				int i;
				for (i = 0; i < field_count; i++)
				{
					raw_row_len += 4;
					if (field_lens[i] > 0)
						raw_row_len += field_lens[i];
				}
				bin_pos += raw_row_len;
			}

			tecc_copy_free_parsed_fields(fields, field_lens, field_count);
			copy_state->current_row++;
		}

		*processed_data = binary_result;
		*processed_len = binary_result_pos;
		return 0;
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

	/* Binary mode: process as binary COPY result (not line-based) */
	if (copy_state->binary_mode)
	{
		char *binary_result = NULL;
		int binary_result_size = 0;
		int binary_result_pos = 0;
		const char *bin_pos = encrypted_data;
		const char *bin_end = encrypted_data + encrypted_len;

		/* Initialize result buffer */
		binary_result_size = encrypted_len * 2 + 1024;
		binary_result = (char *)pg_malloc(binary_result_size);
		if (binary_result == NULL)
		{
			tecc_handle_error(conn, TECC_ERR_MEMORY,
			                  "Failed to allocate memory for binary COPY result");
			return -1;
		}

		/* On first call, skip the binary header (19 bytes) */
		if (copy_state->current_row == 0 && encrypted_len >= TECC_BINARY_HEADER_LEN)
		{
			memcpy(binary_result, bin_pos, TECC_BINARY_HEADER_LEN);
			binary_result_pos = TECC_BINARY_HEADER_LEN;
			bin_pos += TECC_BINARY_HEADER_LEN;
		}

		/* Process remaining data as binary rows */
		while (bin_pos < bin_end)
		{
			int remaining = bin_end - bin_pos;
			uint16 fld_count_net;
			int16 fld_count;
			char **fields = NULL;
			int *field_lens = NULL;
			int field_count = 0;
			int parse_result;

			if (remaining < 2)
				break;

			memcpy(&fld_count_net, bin_pos, 2);
			fld_count = (int16) pg_ntoh16(fld_count_net);

			/* End-of-data marker */
			if (fld_count == -1)
			{
				if (binary_result_pos + 2 > binary_result_size)
				{
					int new_size = binary_result_size * 2;
					char *new_buf = (char *)pg_realloc(binary_result, new_size);
					if (new_buf == NULL) { tecc_pfree(binary_result); return -1; }
					binary_result = new_buf;
					binary_result_size = new_size;
				}
				memcpy(binary_result + binary_result_pos, bin_pos, 2);
				binary_result_pos += 2;
				bin_pos += 2;
				break;
			}

			/* Parse binary row into fields */
			parse_result = tecc_copy_parse_binary_row(bin_pos, remaining,
			                                         &fields, &field_lens,
			                                         &field_count);
			if (parse_result != 0 || fields == NULL)
			{
				tecc_pfree(binary_result);
				tecc_handle_error(conn, TECC_ERR_PROCESSOR,
				                  "Failed to parse binary COPY result row");
				return -1;
			}

			/* Convert binary fields -> TEXT row -> processor -> TEXT -> binary */
			{
				PQExpBufferData text_row_buf;
				char *text_processed = NULL;
				int text_processed_len = 0;
				char **proc_fields = NULL;
				int *proc_field_lens = NULL;
				int proc_field_count = 0;
				char *binary_row = NULL;
				int binary_row_len = 0;
				int i, process_result, parse_proc_result;

				initPQExpBuffer(&text_row_buf);
				for (i = 0; i < field_count; i++)
				{
					if (i > 0)
						appendPQExpBufferChar(&text_row_buf, copy_state->delimiter);

					if (field_lens[i] == -1 || fields[i] == NULL)
						appendPQExpBufferStr(&text_row_buf, "\\N");
					else
					{
						/* ReqID:TECC4.0 - Encrypted: convert net-order binary to host-order hex */
						if (tecc_copy_is_field_encrypted(copy_state, i))
						{
							char *hex_out = NULL;
							if (tecc_copy_cipherdata_to_hex(
								(const unsigned char *)fields[i],
								field_lens[i], &hex_out) == 0)
							{
								appendPQExpBufferStr(&text_row_buf, hex_out);
								tecc_pfree(hex_out);
							}
						}
						else
						{
							/*
							 * ReqID:TECC4.0 - Export: hex-encode non-encrypted raw
							 * bytes WITHOUT the \x prefix, so TECC TEXT parsing
							 * does not consume the backslash as escape char.
							 */
							char *hex_out = NULL;
							if (tecc_hex_encoder_encode(
								(const unsigned char *)fields[i],
								(size_t)field_lens[i],
								&hex_out) == TECC_FORMATTER_SUCCESS && hex_out != NULL)
							{
								const char *hex_p = hex_out;
								if (hex_p[0] == '\\' && hex_p[1] == 'x')
									hex_p += 2;
								appendPQExpBufferStr(&text_row_buf, hex_p);
								tecc_pfree(hex_out);
							}
						}
					}
				}

				/* Process TEXT row through standard processor (export/decrypt) */
				process_result = tecc_process_copy_row(copy_state,
				                                       text_row_buf.data,
				                                       text_row_buf.len,
				                                       &text_processed,
				                                       &text_processed_len,
				                                       false);
				if (process_result != 0)
				{
					termPQExpBuffer(&text_row_buf);
					tecc_copy_free_parsed_fields(fields, field_lens, field_count);
					tecc_pfree(binary_result);
					tecc_handle_error(conn, TECC_ERR_PROCESSOR,
					                  "Failed to process binary COPY result row");
					return -1;
				}

				/* Parse processed TEXT row back into fields */
				parse_proc_result = tecc_copy_parse_text_row(text_processed,
				                                             text_processed_len,
				                                             copy_state->delimiter,
				                                             &proc_fields,
				                                             &proc_field_lens,
				                                             &proc_field_count);
				if (parse_proc_result != 0 || proc_fields == NULL)
				{
					termPQExpBuffer(&text_row_buf);
					tecc_copy_free_parsed_fields(fields, field_lens, field_count);
					tecc_pfree(binary_result);
					tecc_pfree(text_processed);
					tecc_handle_error(conn, TECC_ERR_PROCESSOR,
					                  "Failed to parse processed COPY result row");
					return -1;
				}

				/* ReqID:TECC4.0 - Decode post-TECC fields to proper binary */
				{
					int _di;
					for (_di = 0; _di < proc_field_count; _di++)
					{
						if (proc_fields[_di] == NULL || proc_field_lens[_di] <= 0)
							continue;

						if (tecc_copy_is_field_encrypted(copy_state, _di))
						{
							if (tecc_hex_encoder_has_prefix(proc_fields[_di]) ||
							(proc_fields[_di][0] == 'x' &&
							 proc_fields[_di][1] >= '0' && proc_fields[_di][1] <= '9'))
							{
								unsigned char *raw_bytes = NULL;
								int raw_len = 0;
								if (tecc_copy_hex_to_cipherdata(
										proc_fields[_di],
										&raw_bytes, &raw_len) == 0 &&
								    raw_bytes != NULL && raw_len > 0)
								{
									tecc_pfree(proc_fields[_di]);
									proc_fields[_di] = (char *)raw_bytes;
									proc_field_lens[_di] = raw_len;
								}
							}
							else
							{
								long long llv = atoll(proc_fields[_di]);
								int base_oid = 0;
								int _bj;
								for (_bj = 0; _bj < copy_state->encrypted_column_count; _bj++)
								{
									TeccEncryptedColumnMetadata *col =
										copy_state->encrypted_column_info[_bj];
									if (col)
									{
										int att = copy_state->column_mapping ?
											copy_state->column_mapping[_di] : (_di + 1);
										if (col->column_attnum == att)
										{
											base_oid = col->data_type_original_oid;
											break;
										}
									}
								}
								if (base_oid == 21)
								{
									/* ReqID:TECC4.0 INT2 uses 2 bytes in PG binary COPY */
									uint16 v16 = (uint16)llv;
									uint16 nv = pg_hton16(v16);
									unsigned char *bv = (unsigned char *)pg_malloc(2);
									if (bv)
									{
										memcpy(bv, &nv, 2);
										tecc_pfree(proc_fields[_di]);
										proc_fields[_di] = (char *)bv;
										proc_field_lens[_di] = 2;
									}
								}
								else if (base_oid == 23)
								{
									uint32 uv = pg_hton32((uint32)llv);
									unsigned char *bv = (unsigned char *)pg_malloc(4);
									if (bv)
									{
										memcpy(bv, &uv, 4);
										tecc_pfree(proc_fields[_di]);
										proc_fields[_di] = (char *)bv;
										proc_field_lens[_di] = 4;
									}
								}
								else if (base_oid == 20)
								{
									uint64 uv = (uint64)llv;
									unsigned char bv[8];
									int _bi2;
									unsigned char *bvp = (unsigned char *)pg_malloc(8);
									for (_bi2 = 0; _bi2 < 8; _bi2++)
										bv[7 - _bi2] = (unsigned char)((uv >> (_bi2 * 8)) & 0xff);
									if (bvp)
									{
										memcpy(bvp, bv, 8);
										tecc_pfree(proc_fields[_di]);
										proc_fields[_di] = (char *)bvp;
										proc_field_lens[_di] = 8;
									}
								}
							}
						}
						else
						{
							unsigned char *raw_bytes = NULL;
							size_t raw_len = 0;
							if (tecc_hex_encoder_decode(proc_fields[_di],
								&raw_bytes, &raw_len) == TECC_FORMATTER_SUCCESS &&
							    raw_bytes != NULL && raw_len > 0)
							{
								tecc_pfree(proc_fields[_di]);
								proc_fields[_di] = (char *)raw_bytes;
								proc_field_lens[_di] = (int)raw_len;
							}
						}
					}
				}
				/* Format processed fields back to binary */
				if (tecc_copy_format_binary_row(proc_fields, proc_field_lens,
				                                proc_field_count,
				                                &binary_row, &binary_row_len) != 0)
				{
					termPQExpBuffer(&text_row_buf);
					tecc_copy_free_parsed_fields(fields, field_lens, field_count);
					tecc_copy_free_parsed_fields(proc_fields, proc_field_lens,
					                             proc_field_count);
					tecc_pfree(binary_result);
					tecc_pfree(text_processed);
					tecc_handle_error(conn, TECC_ERR_PROCESSOR,
					                  "Failed to format binary result row");
					return -1;
				}

				/* Append to result */
				while (binary_result_pos + binary_row_len > binary_result_size)
				{
					int new_size = binary_result_size * 2;
					char *new_buf = (char *)pg_realloc(binary_result, new_size);
					if (new_buf == NULL)
					{
						termPQExpBuffer(&text_row_buf);
						tecc_copy_free_parsed_fields(fields, field_lens, field_count);
						tecc_copy_free_parsed_fields(proc_fields, proc_field_lens,
						                             proc_field_count);
						tecc_pfree(binary_result);
						tecc_pfree(binary_row);
						tecc_pfree(text_processed);
						return -1;
					}
					binary_result = new_buf;
					binary_result_size = new_size;
				}
				memcpy(binary_result + binary_result_pos, binary_row,
				       binary_row_len);
				binary_result_pos += binary_row_len;

				tecc_pfree(binary_row);
				tecc_copy_free_parsed_fields(proc_fields, proc_field_lens,
				                             proc_field_count);
				termPQExpBuffer(&text_row_buf);
				tecc_pfree(text_processed);
			}

			/* Advance past the raw binary row */
			{
				int raw_row_len = 2;
				int i;
				for (i = 0; i < field_count; i++)
				{
					raw_row_len += 4;
					if (field_lens[i] > 0)
						raw_row_len += field_lens[i];
				}
				bin_pos += raw_row_len;
			}

			tecc_copy_free_parsed_fields(fields, field_lens, field_count);
			copy_state->current_row++;
		}

		*decrypted_data = binary_result;
		*decrypted_len = binary_result_pos;
		return 0;
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
 * ReqID:TECC4.0 - 单事务恢复(-1)修复：COPY 行到达前，在主连接上补齐本会话
 * 尚未提交（对独立 data-fetcher 连接不可见）的 DEK/KEK 完整缓存项。
 *
 * pg_restore -1 把 CREATE KEK/DEK/TABLE 与 COPY 数据放进同一个未提交事务。
 * CREATE KEK/DEK 后的 tecc_refresh_*_cache 用独立 data-fetcher 连接读元数据，
 * 读不到未提交行，缓存里只剩 PENDING 哨兵；COPY 行流式加密期间主连接忙，
 * 按行回退到独立连接同样读不到 → "DEK search_path not found" → PQputCopyData
 * 失败。表列元数据之所以没这个毛病，是因为 tecc_refresh_table_cache 在 COPY
 * 预处理时用主连接读（同事务可见）。
 *
 * 故在 tecc_create_copy_state（主连接空闲、COPY 尚未真正开始）里，把这些 DEK
 * 及各自 KEK 用主连接读全并塞进客户端缓存，行处理时缓存即命中，不再依赖
 * 独立连接。此函数与 tecc_refresh_table_cache 同法：预处理期间临时关掉
 * enable_tecc 防递归，非管道模式才走（管道下主连接禁止同步 PQexec）。
 */
static TeccDekCacheItem *
tecc_copy_ensure_dek_via_conn(TeccState *state, PGconn *conn,
                              const char *dek_name, const char *ns_hint)
{
	TeccCacheQueryParams q;
	TeccDekCacheItem *item = NULL;
	int count = 0;
	TeccFetcherErrorCode ret;
	bool saved_enable;

	if (state == NULL || conn == NULL || dek_name == NULL || dek_name[0] == '\0')
		return NULL;

	memset(&q, 0, sizeof(q));
	q.flags = TECC_QUERY_BY_NAME;
	q.params.by_name.name = dek_name;
	q.params.by_name.namespace = ns_hint;	/* 可能为 NULL */
	q.external_conn = conn;

	saved_enable = conn->enable_tecc;
	conn->enable_tecc = false;
	ret = tecc_fetch_dek_for_cache(state, &q, &item, &count);
	conn->enable_tecc = saved_enable;

	/* schema 提示未命中（如 DEK 与表不同 schema）时，退化为不限定 namespace */
	if ((ret != TECC_FETCHER_OK || item == NULL) && ns_hint != NULL)
	{
		memset(&q, 0, sizeof(q));
		q.flags = TECC_QUERY_BY_NAME;
		q.params.by_name.name = dek_name;
		q.params.by_name.namespace = NULL;
		q.external_conn = conn;
		saved_enable = conn->enable_tecc;
		conn->enable_tecc = false;
		ret = tecc_fetch_dek_for_cache(state, &q, &item, &count);
		conn->enable_tecc = saved_enable;
	}

	if (ret != TECC_FETCHER_OK || item == NULL)
		return NULL;

	if (item->key_name != NULL && item->key_namespace != NULL)
	{
		/* 先摘旧项（PENDING/负缓存/旧全量项），避免同键重复插入 */
		tecc_cache_remove_pending_dek(state, item->key_name, item->key_namespace);
		if (tecc_cache_insert_dek_item(state, item) != 0)
			tecc_record_debug(state,
				"TECC COPY: insert DEK '%s' into cache failed", item->key_name);
	}
	return item;
}

static TeccKekCacheItem *
tecc_copy_ensure_kek_via_conn(TeccState *state, PGconn *conn,
                              const char *kek_name, const char *ns_hint)
{
	TeccCacheQueryParams q;
	TeccKekCacheItem *item = NULL;
	int count = 0;
	TeccFetcherErrorCode ret;
	bool saved_enable;

	if (state == NULL || conn == NULL || kek_name == NULL || kek_name[0] == '\0')
		return NULL;

	memset(&q, 0, sizeof(q));
	q.flags = TECC_QUERY_BY_NAME;
	q.params.by_name.name = kek_name;
	q.params.by_name.namespace = ns_hint;
	q.external_conn = conn;

	saved_enable = conn->enable_tecc;
	conn->enable_tecc = false;
	ret = tecc_fetch_kek_for_cache(state, &q, &item, &count);
	conn->enable_tecc = saved_enable;

	if ((ret != TECC_FETCHER_OK || item == NULL) && ns_hint != NULL)
	{
		memset(&q, 0, sizeof(q));
		q.flags = TECC_QUERY_BY_NAME;
		q.params.by_name.name = kek_name;
		q.params.by_name.namespace = NULL;
		q.external_conn = conn;
		saved_enable = conn->enable_tecc;
		conn->enable_tecc = false;
		ret = tecc_fetch_kek_for_cache(state, &q, &item, &count);
		conn->enable_tecc = saved_enable;
	}

	if (ret != TECC_FETCHER_OK || item == NULL)
		return NULL;

	if (item->key_name != NULL && item->key_namespace != NULL)
	{
		tecc_cache_remove_pending_kek(state, item->key_name, item->key_namespace);
		if (tecc_cache_insert_kek_item(state, item) != 0)
			tecc_record_debug(state,
				"TECC COPY: insert KEK '%s' into cache failed", item->key_name);
	}
	return item;
}

/*
 * tecc_copy_warm_he_dek - 用主连接预取某 HE DEK 在行级加/解密所需字段。
 *
 * he-add/he-mul 行级加密与导出解密（tecc_crypto_scheduler.c）在 COPY 忙态下无法
 * 用主连接执行 PQexec，字段却只经 tecc_he_load_dek_key_hex（查 hg_dek_args）取。
 * 这里在 COPY 开始前把 per-DEK 字段写入字段级缓存，并像 DET DEK 一样把该 HE DEK
 * 及其 KEK 的全量缓存项补齐，行级 search_path 解析 distributed_id/私钥信封即可命中。
 * 导出纯 HE 值解密时 tecc_he_load_dek_privkey 需读 he_*_privkey 与 kek（DEK 的
 * KEK 名）两行，故除公钥外一并预热。is_ec=1 表示 EC-ElGamal(he-add)，0 表示
 * 有限域 ElGamal(he-mul)。
 */
static void
tecc_copy_warm_he_dek(TeccState *state, PGconn *conn, const char *schema,
                      const char *dek_name, int is_ec)
{
	const char *pub_key = is_ec ? "he_ec_pubkey" : "he_eg_pubkey";
	const char *prv_key = is_ec ? "he_ec_privkey" : "he_eg_privkey";
	char  *hex = NULL;
	TeccDekCacheItem *dek_item;

	if (state == NULL || conn == NULL || dek_name == NULL || dek_name[0] == '\0')
		return;

	if (tecc_he_load_dek_key_hex(state, dek_name, pub_key, &hex) != 0)
		tecc_record_debug(state,
			"TECC COPY: warm %s for HE DEK '%s' not found", pub_key, dek_name);
	if (hex != NULL)
		pg_free(hex);
	hex = NULL;

	if (tecc_he_load_dek_key_hex(state, dek_name, prv_key, &hex) != 0)
		tecc_record_debug(state,
			"TECC COPY: warm %s for HE DEK '%s' not found", prv_key, dek_name);
	if (hex != NULL)
		pg_free(hex);
	hex = NULL;

	/* 私钥信封解封（SM2 解封）需该 DEK 的 kek 名（hg_dek_args 'kek' 行） */
	if (tecc_he_load_dek_key_hex(state, dek_name, "kek", &hex) != 0)
		tecc_record_debug(state,
			"TECC COPY: warm kek for HE DEK '%s' not found", dek_name);
	if (hex != NULL)
		pg_free(hex);

	dek_item = tecc_copy_ensure_dek_via_conn(state, conn, dek_name, schema);
	if (dek_item != NULL && dek_item->kek_name != NULL)
		tecc_copy_ensure_kek_via_conn(state, conn, dek_item->kek_name, schema);
}

/*
 * tecc_copy_preload_table_keys - 在 COPY 开始前用主连接补全目标表的 DEK/KEK
 */
static void
tecc_copy_preload_table_keys(TeccEnhancedCopyState *copy_state)
{
	PGconn *conn;
	TeccState *state;
	const char *db;
	const char *schema;
	const char *table;
	TeccColumnCacheItem **cols;
	int ncols;
	int i;

	if (copy_state == NULL)
		return;
	conn = copy_state->conn;
	if (conn == NULL)
		return;
	/* 管道模式下主连接禁止同步 PQexec：保持原行为（独立连接看不到未提交 DDL） */
	if (conn->pipelineStatus != PQ_PIPELINE_OFF)
		return;
	state = tecc_get_state(conn);
	if (state == NULL)
		return;
	schema = copy_state->schema_name;
	table = copy_state->table_name;
	if (schema == NULL || table == NULL)
		return;
	db = PQdb(conn);
	if (db == NULL || db[0] == '\0')
		db = "postgres";

	cols = tecc_cache_get_columns_by_table_name(state, db, schema, table, &ncols);
	if (cols == NULL || ncols <= 0)
		return;

	for (i = 0; i < ncols; i++)
	{
		TeccColumnCacheItem *col = cols[i];
		TeccDekCacheItem *dek_item;

		if (col == NULL)
			continue;

		/* 对称/DET DEK + 其 KEK（纯 HE 列 dek_name 为 NULL，跳过） */
		if (col->dek_name != NULL && col->dek_name[0] != '\0')
		{
			dek_item = tecc_copy_ensure_dek_via_conn(state, conn, col->dek_name, schema);
			if (dek_item != NULL && dek_item->kek_name != NULL)
				tecc_copy_ensure_kek_via_conn(state, conn, dek_item->kek_name, schema);
		}

		/* 一列多 DEK / HE 列：预热 he-add/he-mul 所需的字段级缓存（列 HE DEK 名 +
		 * per-DEK 公钥）及该 HE DEK/KEK 的全量缓存项。行级加密在 COPY 忙态下不能
		 * 用主连接查 hg_encrypted_column_args / hg_dek_args，故须提前取好。 */
		if ((col->encryption_type & 0x08) != 0 ||	/* he-add */
			(col->encryption_type & 0x10) != 0 ||	/* he-mul */
			(col->he_add_dek_name != NULL && col->he_add_dek_name[0] != '\0') ||
			(col->he_mul_dek_name != NULL && col->he_mul_dek_name[0] != '\0'))
		{
			char he_add_buf[NAMEDATALEN] = {0};
			char he_mul_buf[NAMEDATALEN] = {0};
			const char *add_dek = NULL;
			const char *mul_dek = NULL;

			/* he-add：DEK 名优先取列绑定（与行级 tecc_load_column_he_dek_name +
			 * context->dek_name 回退一致），公钥字段 key='he_ec_pubkey' */
			if ((col->encryption_type & 0x08) != 0 ||
				(col->he_add_dek_name != NULL && col->he_add_dek_name[0] != '\0'))
			{
				if (OidIsValid(col->table_oid) && col->column_attnum > 0)
					tecc_load_column_he_dek_name(state, col->table_oid,
												 col->column_attnum, "he_add_dek",
												 he_add_buf, sizeof(he_add_buf));
				add_dek = (he_add_buf[0] != '\0') ? he_add_buf :
					((col->he_add_dek_name != NULL && col->he_add_dek_name[0] != '\0') ?
					 col->he_add_dek_name : col->dek_name);
				tecc_copy_warm_he_dek(state, conn, schema, add_dek, 1);
			}

			/* he-mul：公钥字段 key='he_eg_pubkey' */
			if ((col->encryption_type & 0x10) != 0 ||
				(col->he_mul_dek_name != NULL && col->he_mul_dek_name[0] != '\0'))
			{
				if (OidIsValid(col->table_oid) && col->column_attnum > 0)
					tecc_load_column_he_dek_name(state, col->table_oid,
												 col->column_attnum, "he_mul_dek",
												 he_mul_buf, sizeof(he_mul_buf));
				mul_dek = (he_mul_buf[0] != '\0') ? he_mul_buf :
					((col->he_mul_dek_name != NULL && col->he_mul_dek_name[0] != '\0') ?
					 col->he_mul_dek_name : col->dek_name);
				tecc_copy_warm_he_dek(state, conn, schema, mul_dek, 0);
			}
		}
	}

	/* DEK 补全后，把先前因缓存未满而为 NULL 的列 DEK 名回填，让
	 * tecc_copy_state_init_crypto_context 的 KEK 解析能继续 */
	if (copy_state->encrypted_column_info != NULL)
	{
		for (i = 0; i < copy_state->encrypted_column_count; i++)
		{
			TeccEncryptedColumnMetadata *meta = copy_state->encrypted_column_info[i];
			TeccDekCacheItem *item;

			if (meta == NULL || meta->column_key_id == InvalidOid || meta->dek_name != NULL)
				continue;
			item = tecc_cache_get_dek_by_oid(state, meta->column_key_id);
			if (item != NULL && item->key_name != NULL)
				meta->dek_name = pg_strdup(item->key_name);
		}
	}
}

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
	char *schema_name = NULL;
	char **column_names = NULL;
	int column_count = 0;
	bool is_from_stdin = false;
	bool is_to_stdout = false;
	int i;

	/* Parse COPY command with format options */
	bool csv_mode = false;
	bool binary_mode = false;
	char delimiter = '\t';
	char quote = '"';
	char escape = '\\';
	bool header = false;
	char *null_string = NULL;

	int *subquery_mapping = NULL;
	int subquery_mapping_count = 0;
	char **subquery_column_table_names = NULL;

	if (tecc_parse_copy_with_frontend_parser(conn, command, &table_name, &column_names,
									   &column_count,
									   &subquery_mapping, &subquery_mapping_count,
									   &subquery_column_table_names,
									   &is_from_stdin, &is_to_stdout,
									   &csv_mode, &binary_mode,
					   &delimiter, &quote, &escape,
									   &header, &null_string,
									   &schema_name) != 0)
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
	copy_state->schema_name = schema_name;  /* 可能为 NULL（未限定表名，运行时解析） */
	copy_state->column_names = column_names;
	copy_state->column_count = column_count;
	/* For subquery with column_names populated, set column_count for processor name matching */
	if (subquery_mapping != NULL && subquery_mapping_count > 0 && copy_state->column_names != NULL &&
		copy_state->column_count == 0)
		copy_state->column_count = subquery_mapping_count;
	copy_state->is_from_stdin = is_from_stdin;
	copy_state->is_to_stdout = is_to_stdout;
	copy_state->current_row = 0;

	/* Set default format values */
	copy_state->delimiter = delimiter;
	copy_state->quote = quote;
	copy_state->escape = escape;
	copy_state->csv_mode = csv_mode;
	copy_state->binary_mode = binary_mode;
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

	{
	}
	/* Initialize column mapping: use subquery mapping if available */
	if (subquery_mapping != NULL && subquery_mapping_count > 0)
	{
		copy_state->column_mapping = subquery_mapping;
		copy_state->mapping_count = subquery_mapping_count;
		copy_state->column_table_names = subquery_column_table_names;
	}
	else if (tecc_copy_state_init_column_mapping(copy_state) != 0)
	{
		tecc_record_debug(NULL, "Failed to initialize column mapping");
		/* Continue anyway, mapping will be identity mapping */
	}

	/* For multi-table subquery: load encrypted columns from all referenced tables */
	(void)tecc_copy_state_load_mapped_tables((TeccEnhancedCopyState *)copy_state);

	/* ReqID:TECC4.0 - 单事务恢复(-1)修复：行到达前用主连接补全本会话未提交的
	 * DEK/KEK 缓存（详见 tecc_copy_preload_table_keys 注释）。必须在
	 * init_crypto_context 之前，且 COPY 尚未把主连接置为 COPY-in 忙态。 */
	tecc_copy_preload_table_keys((TeccEnhancedCopyState *)copy_state);

	/* Initialize crypto context */
	if (tecc_copy_state_init_crypto_context(copy_state) != 0)
	{
		tecc_record_debug(NULL, "Failed to initialize crypto context");
		/* Continue without crypto context */
	}

	return copy_state;

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
	if (subquery_mapping != NULL)
	{
		tecc_pfree(subquery_mapping);
	}
	if (subquery_column_table_names != NULL)
	{
		tecc_pfree(subquery_column_table_names);
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
		return;
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

	/* Free column table names */
	if (copy_state->column_table_names != NULL)
	{
		/* ReqID:TECC4.0 - Bug 907：逐列来源表名由 processor pg_strdup 出来 */
		for (i = 0; i < copy_state->mapping_count; i++)
		{
			if (copy_state->column_table_names[i] != NULL)
			{
				tecc_pfree(copy_state->column_table_names[i]);
			}
		}
		tecc_pfree(copy_state->column_table_names);
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

	/* Free crypto context */
	if (copy_state->crypto_ctx != NULL)
	{
		tecc_copy_state_cleanup_crypto_context((TeccEnhancedCopyState *)copy_state);
	}

	/* Free COPY state itself */
	tecc_pfree(copy_state);
}


/*
 * ReqID:TECC4.0 - Bug 907：取 FROM 项里最左侧的 RangeVar。
 *
 * 显式 JOIN 的 fromClause 首项是 JoinExpr（而不是 RangeVar），原实现只判
 * 首项类型，导致 \copy (SELECT ... JOIN ...) TO 的 COPY 状态拿不到表名，
 * 加密列元数据一条也加载不到，导出整行输出密文。
 * 这里沿 JOIN 左子树递归下降到最左表。
 */
static const RangeVar *
tecc_copy_first_rangevar(const Node *node)
{
	if (node == NULL)
		return NULL;

	if (nodeTag(node) == FE_T_RangeVar)
		return (const RangeVar *) node;

	if (nodeTag(node) == FE_T_JoinExpr)
	{
		const JoinExpr *join = (const JoinExpr *) node;
		const RangeVar *rv = tecc_copy_first_rangevar(join->larg);

		if (rv != NULL)
			return rv;
		return tecc_copy_first_rangevar(join->rarg);
	}

	return NULL;
}

/*
 * tecc_parse_copy_subquery - Extract table and column mapping from subquery
 *
 * For \copy (SELECT ...) TO STDOUT: extracts the table name and builds
 * a column_mapping array (1-based attnums) for encrypted column detection.
 */
static int
tecc_parse_copy_with_frontend_parser(PGconn *conn, const char *command, char **table_name,
                                     char ***column_names, int *column_count,
                                     int **column_mapping, int *mapping_count,
                                     char ***column_table_names,
                                     bool *is_from_stdin, bool *is_to_stdout,
                                     bool *csv_mode, bool *binary_mode,
                                     char *delimiter,
                                     char *quote, char *escape,
                                     bool *header, char **null_string,
                                     char **schema_name)
{
	Node *parse_tree = NULL;
	ParserError *errors = NULL;
	CopyStmt *copy_stmt = NULL;

	/* Initialize output parameters */
	if (table_name) *table_name = NULL;
	if (schema_name) *schema_name = NULL;
	if (column_names) *column_names = NULL;
	if (column_count) *column_count = 0;
	if (column_mapping) *column_mapping = NULL;
	if (mapping_count) *mapping_count = 0;
	if (column_table_names) *column_table_names = NULL;
	if (is_from_stdin) *is_from_stdin = false;
	if (is_to_stdout) *is_to_stdout = false;
	if (csv_mode) *csv_mode = false;
	if (binary_mode) *binary_mode = false;
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


	/* ReqID:TECC4.0 — Handle \copy (SELECT ...) TO STDOUT with subquery */
	if (copy_stmt->relation == NULL && copy_stmt->query != NULL && !copy_stmt->is_from)
	{
		/* Extract table_name for state initialization (load_encrypted_columns needs it).
		 * Column mapping is built by the processor (tecc_stmt_processor_process_copy
		 * → process_select) and propagated to copy_state via the hook. */
		if (table_name && copy_stmt->query &&
			nodeTag(copy_stmt->query) == FE_T_SelectStmt)
		{
			SelectStmt *sel = (SelectStmt *)copy_stmt->query;
			if (sel->fromClause != NULL &&
				list_length(sel->fromClause) > 0)
			{
				const RangeVar *rv =
					tecc_copy_first_rangevar((const Node *)linitial(sel->fromClause));
				if (rv != NULL && rv->relname != NULL)
					*table_name = tecc_strdup(rv->relname);
			}
		}
		/* Set output direction */
		if (is_to_stdout)
			*is_to_stdout = !copy_stmt->is_from;
		if (null_string)
			*null_string = NULL;
		/* Fall through to parse format options (CSV, delimiter, etc.) */
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

	/* ReqID:TECC4.0 - 提取显式 schema（未限定表名时为 NULL，由调用方按 search_path 解析） */
	if (schema_name && copy_stmt->relation && copy_stmt->relation->schemaname)
	{
		*schema_name = tecc_strdup(copy_stmt->relation->schemaname);
		if (*schema_name == NULL)
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
				if (format)
				{
					if (csv_mode)
						*csv_mode = (strcasecmp(format, "csv") == 0);
					if (binary_mode)
						*binary_mode = (strcasecmp(format, "binary") == 0);
					/* Set CSV defaults if CSV mode */
					if (csv_mode && *csv_mode && delimiter)
						*delimiter = ',';
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

	if (column_mapping && *column_mapping)
	{
		tecc_pfree(*column_mapping);
		*column_mapping = NULL;
		if (mapping_count)
			*mapping_count = 0;
	}

	if (parse_tree) freeNode(parse_tree);
	return -1;
}

/* End - ReqID:TECC4.0 - Use frontend parser for COPY command parsing */

/* Begin - ReqID:TECC4.0 - Enhanced COPY row processing */

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
    if (copy_state->binary_mode) {
        /* Binary format: parse using length-prefixed field format */
        result = tecc_copy_parse_binary_row(row_data, row_len,
                                           &fields, &field_lens, &field_count);
    } else if (copy_state->csv_mode) {
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
    if (copy_state->binary_mode) {
        result = tecc_copy_format_binary_row(fields, field_lens, field_count,
                                            &formatted_row, &formatted_len);
    } else if (copy_state->csv_mode) {
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

	/* ReqID:TECC4.0 - 未限定表名按 search_path 解析真实 schema（同 stmt_load_table_metadata）；
	 * 显式限定名（copy_state->schema_name 非空）直接使用 */
	{
		TeccStatementData tmp_data;
		memset(&tmp_data, 0, sizeof(tmp_data));
		tmp_data.conn = copy_state->conn;
		schema_name = tecc_stmt_resolve_table_schema(&tmp_data,
													 copy_state->schema_name,
													 copy_state->table_name);
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
				column_meta->table_name = copy_state->table_name ? pg_strdup(copy_state->table_name) : NULL;
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
						column_meta->table_name = copy_state->table_name ? pg_strdup(copy_state->table_name) : NULL;
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
 * tecc_copy_state_load_encrypted_columns_for_table - Load encrypted columns from cache
 * for a specific table and merge into copy_state.
 */
static int
tecc_copy_state_load_encrypted_columns_for_table(TeccEnhancedCopyState *copy_state,
                                                  const char *table_name)
{
	TeccState *state;
	const char *database_name = NULL;
	const char *schema_name;
	TeccColumnCacheItem **cached_columns;
	int cached_column_count;
	int old_count;
	int i;

	if (copy_state == NULL || table_name == NULL)
		return -1;

	state = tecc_get_state(copy_state->conn);
	if (state == NULL)
		return -1;

	if (copy_state->conn != NULL)
		database_name = PQdb(copy_state->conn);
	if (database_name == NULL || database_name[0] == '\0')
		database_name = "postgres";
	/* ReqID:TECC4.0 - 未限定表名按 search_path 解析真实 schema（同 stmt_load_table_metadata） */
	{
		TeccStatementData tmp_data;
		memset(&tmp_data, 0, sizeof(tmp_data));
		tmp_data.conn = copy_state->conn;
		schema_name = tecc_stmt_resolve_table_schema(&tmp_data,
													 copy_state->schema_name,
													 table_name);
	}

	cached_columns = tecc_cache_get_columns_by_table_name(state, database_name,
		schema_name, table_name, &cached_column_count);
	if (cached_columns == NULL || cached_column_count <= 0)
		return 0;

	old_count = copy_state->encrypted_column_count;
	{
		int new_count = old_count + cached_column_count;
		TeccEncryptedColumnMetadata **new_info;
		if (old_count == 0)
			new_info = (TeccEncryptedColumnMetadata **)pg_malloc(
				new_count * sizeof(TeccEncryptedColumnMetadata *));
		else
			new_info = (TeccEncryptedColumnMetadata **)pg_realloc(
				copy_state->encrypted_column_info,
				new_count * sizeof(TeccEncryptedColumnMetadata *));
		if (new_info == NULL)
			return -1;
		copy_state->encrypted_column_info = new_info;
		copy_state->encrypted_column_count = new_count;
	}
	for (i = 0; i < cached_column_count; i++)
	{
		TeccColumnCacheItem *src = cached_columns[i];
		TeccEncryptedColumnMetadata *meta;
		if (src == NULL || src->column_name == NULL)
		{
			copy_state->encrypted_column_count--;
			continue;
		}
		meta = (TeccEncryptedColumnMetadata *)
			pg_malloc(sizeof(TeccEncryptedColumnMetadata));
		if (meta == NULL)
		{
			copy_state->encrypted_column_count--;
			continue;
		}
		memset(meta, 0, sizeof(TeccEncryptedColumnMetadata));
		meta->table_oid = src->table_oid;
		meta->column_attnum = src->column_attnum;
		meta->column_name = pg_strdup(src->column_name);
		meta->table_name = pg_strdup(table_name);
		meta->data_type_original_oid = src->original_type_oid;
		meta->data_type_original_mod = src->original_type_mod;
		meta->dek_name = NULL;
		if (state != NULL && src->dek_id != InvalidOid)
		{
			TeccDekCacheItem *dek_item = tecc_cache_get_dek_by_oid(state, src->dek_id);
			if (dek_item == NULL && src->dek_name != NULL)
				dek_item = tecc_cache_get_dek_by_name_search_path(state, src->dek_name);
			if (dek_item != NULL && dek_item->key_name != NULL)
				meta->dek_name = pg_strdup(dek_item->key_name);
		}
		meta->encryption_type = src->encryption_type;
		meta->column_key_id = src->dek_id;
		meta->enable_det = (src->encryption_type & 0x01) != 0;
		meta->enable_ope = (src->encryption_type & 0x02) != 0;
		meta->enable_search = (src->encryption_type & 0x04) != 0;
		meta->collation_oid = src->collation_oid;
		meta->collation_locale = src->collation_locale ? pg_strdup(src->collation_locale) : NULL;
		meta->create_date = src->create_date;
		meta->creator = 0;
		copy_state->encrypted_column_info[old_count + i] = meta;
	}
	return 0;
}

/*
 * ReqID:TECC4.0 - Bug 907：按当前列映射补载"其余来源表"的密态列元数据。
 *
 * 多表 JOIN 子查询的来源表名只有 processor（tecc_stmt_processor_process_copy）
 * 解析 SELECT 才能得到，而 COPY 状态（连同加密列元数据）在它之前就建好了，
 * 表名经 hook 事后回填。所以 JOIN 右表的元数据只能在这里补载 —— 不补载就
 * 匹配不到该表的密态列，导出整列输出密文。
 *
 * 幂等：已加载过元数据的表直接跳过。返回本次新加载（列数真正增加）的表数。
 */
int
tecc_copy_state_load_mapped_tables(TeccEnhancedCopyState *copy_state)
{
	int i;
	int loaded = 0;

	if (copy_state == NULL || copy_state->column_table_names == NULL ||
		copy_state->mapping_count <= 0)
		return 0;

	for (i = 0; i < copy_state->mapping_count; i++)
	{
		const char *table_name = copy_state->column_table_names[i];
		int before = copy_state->encrypted_column_count;
		bool already = false;
		int j;

		if (table_name == NULL)
			continue;

		/* 该表是否已有元数据（主表在 create_copy_state 里已载入） */
		for (j = 0; j < copy_state->encrypted_column_count; j++)
		{
			TeccEncryptedColumnMetadata *meta = copy_state->encrypted_column_info[j];
			if (meta != NULL && meta->table_name != NULL &&
				pg_strcasecmp(meta->table_name, table_name) == 0)
			{
				already = true;
				break;
			}
		}
		if (already)
			continue;

		if (tecc_copy_state_load_encrypted_columns_for_table(copy_state,
		                                                     table_name) != 0)
		{
			TeccState *state = (copy_state->conn != NULL)
			                   ? tecc_get_state(copy_state->conn) : NULL;
			tecc_record_debug(state,
				"Failed to load encrypted columns for mapped table: %s", table_name);
			continue;
		}
		if (copy_state->encrypted_column_count > before)
			loaded++;
	}

	return loaded;
}

/*
 * ReqID:TECC4.0 - Bug 907：补载映射表，并把"按列数定长分配"的加解密上下文重建。
 *
 * encrypted_column_info 之外，crypto_ctx / 流式上下文数组的长度都是
 * encrypted_column_count，销毁时也按该值遍历；补载会让列数变大，所以必须先
 * 按旧列数销毁、后按新列数重建，否则销毁时会越过原数组末尾读。
 */
int
tecc_copy_state_reload_mapped_columns(TeccEnhancedCopyState *copy_state)
{
	int loaded;

	if (copy_state == NULL)
		return 0;

	tecc_copy_state_cleanup_crypto_context(copy_state);
	tecc_copy_state_cleanup_stream_contexts(copy_state);

	loaded = tecc_copy_state_load_mapped_tables(copy_state);

	if (loaded > 0)
		(void)tecc_copy_state_init_crypto_context(copy_state);

	return loaded;
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

	/* 初始化列映射：文件列i映射到表列(i+1)（attnum是1-based） */
	for (i = 0; i < copy_state->column_count; i++)
	{
		column_mapping[i] = i + 1;
	}

	/* 更新COPY状态 */
	copy_state->column_mapping = column_mapping;
	copy_state->mapping_count = copy_state->column_count;

	/* 记录调试信息 */
	tecc_record_debug(NULL, "Initialized column mapping (identity mapping)");

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

/*
 * tecc_copy_parse_binary_row - Parse a binary COPY row into fields
 *
 * In PostgreSQL binary COPY format, each row consists of:
 *   - int16 field_count (network byte order)
 *   - for each field: int32 length (network byte order, -1 = NULL) followed by data
 *
 * Returns: 0 on success, -1 on failure
 */
static int
tecc_copy_parse_binary_row(const char *row_data, int row_len,
                           char ***fields, int **field_lens, int *field_count)
{
	const char *pos = row_data;
	const char *end = row_data + row_len;
	char **field_array = NULL;
	int *len_array = NULL;
	int max_fields = 64;
	int field_idx = 0;
	uint16 fld_count_net;
	int16 fld_count;

	if (row_data == NULL || row_len <= 0 || fields == NULL ||
		field_lens == NULL || field_count == NULL)
		return -1;

	if (row_len < 2)
		return -1;

	/* Read field count (int16, network byte order) */
	memcpy(&fld_count_net, pos, 2);
	fld_count = (int16) pg_ntoh16(fld_count_net);
	pos += 2;

	/* -1 field count means end-of-data marker */
	if (fld_count == -1)
	{
		*fields = NULL;
		*field_lens = NULL;
		*field_count = 0;
		return 0;
	}

	max_fields = (fld_count > 0) ? fld_count : 64;

	field_array = (char **)pg_malloc(max_fields * sizeof(char *));
	len_array = (int *)pg_malloc(max_fields * sizeof(int));
	if (field_array == NULL || len_array == NULL)
	{
		if (field_array) tecc_pfree(field_array);
		if (len_array) tecc_pfree(len_array);
		return -1;
	}
	memset(field_array, 0, max_fields * sizeof(char *));

	/* Read each field */
	while (field_idx < fld_count && pos + 4 <= end)
	{
		uint32 field_len_net;
		int32 field_len;

		memcpy(&field_len_net, pos, 4);
		field_len = (int32) pg_ntoh32(field_len_net);
		pos += 4;

		if (field_len == -1)
		{
			len_array[field_idx] = -1;
			field_array[field_idx] = NULL;
			field_idx++;
			continue;
		}

		if (field_len < 0 || pos + field_len > end)
		{
			tecc_copy_free_parsed_fields(field_array, len_array, field_idx);
			return -1;
		}

		field_array[field_idx] = (char *)pg_malloc(field_len);
		if (field_array[field_idx] == NULL)
		{
			tecc_copy_free_parsed_fields(field_array, len_array, field_idx);
			return -1;
		}
		memcpy(field_array[field_idx], pos, field_len);
		len_array[field_idx] = field_len;

		pos += field_len;
		field_idx++;
	}

	if (field_idx != fld_count)
	{
		tecc_copy_free_parsed_fields(field_array, len_array, field_idx);
		return -1;
	}

	*fields = field_array;
	*field_lens = len_array;
	*field_count = fld_count;
	return 0;
}

/*
 * tecc_copy_format_binary_row - Format fields into binary COPY row
 *
 * Returns: 0 on success, -1 on failure
 */
static int
tecc_copy_format_binary_row(char **fields, int *field_lens, int field_count,
                            char **row_data, int *row_len)
{
	char *result;
	int pos = 0;
	int total_size;
	int i;

	if (fields == NULL || field_lens == NULL || field_count < 0 ||
		row_data == NULL || row_len == NULL)
		return -1;

	/* Calculate total size */
	total_size = 2;
	for (i = 0; i < field_count; i++)
	{
		total_size += 4;
		if (field_lens[i] > 0)
			total_size += field_lens[i];
	}

	result = (char *)pg_malloc(total_size);
	if (result == NULL)
		return -1;

	/* Write field count (int16, network byte order) */
	{
		uint16 fld_count_net = pg_hton16((uint16) field_count);
		memcpy(result + pos, &fld_count_net, 2);
		pos += 2;
	}

	/* Write each field */
	for (i = 0; i < field_count; i++)
	{
		uint32 field_len_net;

		if (field_lens[i] == -1 || fields[i] == NULL)
		{
			field_len_net = pg_hton32((uint32) -1);
			memcpy(result + pos, &field_len_net, 4);
			pos += 4;
		}
		else
		{
			field_len_net = pg_hton32((uint32) field_lens[i]);
			memcpy(result + pos, &field_len_net, 4);
			pos += 4;
			memcpy(result + pos, fields[i], field_lens[i]);
			pos += field_lens[i];
		}
	}

	*row_data = result;
	*row_len = total_size;
	return 0;
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
		return -1;
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
		return -1;
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
		return -1;
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
		return -1;
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
		return -1;
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
		return -1;
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
		return -1;
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
		return -1;
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
