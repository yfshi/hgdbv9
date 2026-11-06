#include "postgres_fe.h"
#include "common/fe_memutils.h"
#include "libpq-fe.h"  /* For PQdb function */
/* ReqID:TECC4.0 - Encrypted type OID constants */
#include "catalog/pg_type_enc.h"
/* Begin - ReqID:TECC4.0 */
/*
 * File: record_processor.c
 *
 * Abstract:
 *   TECC record processor implementation for query result decryption.
 *   Handles decryption of encrypted data in query result records.
 *
 * Authored by <wangxp@highgo.com>, 20260410.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/record_processor.c
 */

#include "record_processor.h"
#include "internal/tecc_processor_internal.h"
#include "../tecc_scheduler/tecc_crypto_scheduler.h"  /* Crypto scheduler interface */
#include "../tecc_cache/tecc_metadata_cache.h"        /* Metadata cache interface */
#include "../tecc_libpq_integration/fe-tecc-integration.h"  /* For tecc_get_state() */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Internal helper functions */
static TeccRecordFieldInfo *create_field_info(PGconn *conn, PGresult *result, int field_index);
static void free_field_info(TeccRecordFieldInfo *field_info);
static TeccRecordProcessorResult decrypt_field_value(PGconn *conn,
                                                    TeccRecordFieldInfo *field_info,
                                                    const char *encrypted_value,
                                                    char **decrypted_value);
static bool is_encrypted_type_oid(Oid type_oid);

/* Initialize record processor context */
TeccRecordProcessorContext *
tecc_record_processor_init(PGconn *conn, PGresult *result)
{
    TeccRecordProcessorContext *context;
    int num_fields;
    int i;

    if (conn == NULL || result == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid input parameters to record processor init");
        return NULL;
    }

    /* Allocate context */
    context = (TeccRecordProcessorContext *)pg_malloc(sizeof(TeccRecordProcessorContext));
    if (context == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to allocate record processor context");
        return NULL;
    }

    /* Initialize context */
    memset(context, 0, sizeof(TeccRecordProcessorContext));
    context->conn = conn;
    context->result = result;
    context->current_row = -1;
    context->error_code = TECC_RECORD_PROCESSOR_SUCCESS;

    /* Get number of fields in result */
    num_fields = PQnfields(result);
    if (num_fields <= 0) {
        tecc_processor_debug(TECC_LOG_WARNING,
                           "Result has no fields");
        context->num_fields = 0;
        return context;
    }

    context->num_fields = num_fields;

    /* Allocate field info array */
    context->field_info = (TeccRecordFieldInfo **)pg_malloc(
        sizeof(TeccRecordFieldInfo *) * num_fields);
    if (context->field_info == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to allocate field info array");
        pg_free(context);
        return NULL;
    }

    /* Initialize field info array to NULL */
    for (i = 0; i < num_fields; i++) {
        context->field_info[i] = NULL;
    }

    /* Count encrypted fields */
    context->num_encrypted_fields = 0;

    

    return context;
}

/* Process a single record (row) */
TeccRecordProcessorResult
tecc_record_processor_process_row(
    TeccRecordProcessorContext *context,
    int row_index,
    char ***decrypted_values)
{
    int num_fields;
    int i;
    char **values;
    TeccRecordProcessorResult result;

    if (context == NULL || decrypted_values == NULL) {
        return TECC_RECORD_PROCESSOR_INVALID_INPUT;
    }

    if (row_index < 0 || row_index >= PQntuples(context->result)) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid row index: %d (total rows: %d)",
                           row_index, PQntuples(context->result));
        return TECC_RECORD_PROCESSOR_INVALID_INPUT;
    }

    context->current_row = row_index;
    num_fields = context->num_fields;

    /* Allocate decrypted values array */
    values = (char **)pg_malloc(sizeof(char *) * num_fields);
    if (values == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to allocate decrypted values array");
        return TECC_RECORD_PROCESSOR_MEMORY_ERROR;
    }

    /* Initialize array to NULL */
    for (i = 0; i < num_fields; i++) {
        values[i] = NULL;
    }

    /* Process each field */
    for (i = 0; i < num_fields; i++) {
        const char *field_value;
        bool is_null;

        /* Get field value from result */
        field_value = PQgetvalue(context->result, row_index, i);
        is_null = PQgetisnull(context->result, row_index, i);

        if (is_null) {
            /* NULL value - no decryption needed */
            values[i] = NULL;
            continue;
        }

        /* Check if field is encrypted */
        if (context->field_info[i] == NULL) {
            /* Lazy initialization of field info */
            context->field_info[i] = create_field_info(context->conn,
                                                      context->result, i);
            if (context->field_info[i] == NULL) {
                tecc_processor_debug(TECC_LOG_ERROR,
                                   "Failed to create field info for field %d", i);
                /* Clean up allocated values */
                {
                    int j;
                    for (j = 0; j < i; j++) {
                        if (values[j] != NULL) {
                            pg_free(values[j]);
                        }
                    }
                }
                pg_free(values);
                return TECC_RECORD_PROCESSOR_ERROR;
            }

            if (context->field_info[i]->is_encrypted) {
                context->num_encrypted_fields++;
            }
        }

        if (context->field_info[i]->is_encrypted) {
            /* Decrypt encrypted field */
            result = decrypt_field_value(context->conn,
                                        context->field_info[i],
                                        field_value,
                                        &values[i]);
            if (result != TECC_RECORD_PROCESSOR_SUCCESS) {
                tecc_processor_debug(TECC_LOG_ERROR,
                                   "Failed to decrypt field %d: error %d",
                                   i, result);
                /* Clean up allocated values */
                {
                    int j;
                    for (j = 0; j < i; j++) {
                        if (values[j] != NULL) {
                            pg_free(values[j]);
                        }
                    }
                }
                pg_free(values);
                return result;
            }
        } else {
            /* Non-encrypted field - copy value */
            values[i] = pg_strdup(field_value);
            if (values[i] == NULL) {
                tecc_processor_debug(TECC_LOG_ERROR,
                                   "Failed to copy non-encrypted field %d", i);
                /* Clean up allocated values */
                {
                    int j;
                    for (j = 0; j < i; j++) {
                        if (values[j] != NULL) {
                            pg_free(values[j]);
                        }
                    }
                }
                pg_free(values);
                return TECC_RECORD_PROCESSOR_MEMORY_ERROR;
            }
        }
    }

    *decrypted_values = values;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Successfully processed row %d, %d encrypted fields decrypted",
                       row_index, context->num_encrypted_fields);

    return TECC_RECORD_PROCESSOR_SUCCESS;
}

/* Process multiple records in batch */
TeccRecordProcessorResult
tecc_record_processor_process_batch(
    TeccRecordProcessorContext *context,
    int start_row,
    int num_rows,
    char ****decrypted_values)
{
    int total_rows;
    int i;
    char ***batch_values;
    TeccRecordProcessorResult result;

    if (context == NULL || decrypted_values == NULL) {
        return TECC_RECORD_PROCESSOR_INVALID_INPUT;
    }

    total_rows = PQntuples(context->result);
    if (start_row < 0 || start_row >= total_rows ||
        num_rows <= 0 || start_row + num_rows > total_rows) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid batch parameters: start=%d, num=%d, total=%d",
                           start_row, num_rows, total_rows);
        return TECC_RECORD_PROCESSOR_INVALID_INPUT;
    }

    /* Allocate batch values array */
    batch_values = (char ***)pg_malloc(sizeof(char **) * num_rows);
    if (batch_values == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to allocate batch values array");
        return TECC_RECORD_PROCESSOR_MEMORY_ERROR;
    }

    /* Initialize array to NULL */
    for (i = 0; i < num_rows; i++) {
        batch_values[i] = NULL;
    }

    /* Process each row in batch */
    for (i = 0; i < num_rows; i++) {
        int row_index = start_row + i;

        result = tecc_record_processor_process_row(context,
                                                  row_index,
                                                  &batch_values[i]);
        if (result != TECC_RECORD_PROCESSOR_SUCCESS) {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "Failed to process row %d in batch: error %d",
                               row_index, result);
            /* Clean up allocated batch values */
            {
                int j, k;
                for (j = 0; j < i; j++) {
                    if (batch_values[j] != NULL) {
                        int num_fields = context->num_fields;
                        for (k = 0; k < num_fields; k++) {
                            if (batch_values[j][k] != NULL) {
                                pg_free(batch_values[j][k]);
                            }
                        }
                        pg_free(batch_values[j]);
                    }
                }
            }
            pg_free(batch_values);
            return result;
        }
    }

    *decrypted_values = batch_values;

    

    return TECC_RECORD_PROCESSOR_SUCCESS;
}

/* Get field information for a specific column */
TeccRecordFieldInfo *
tecc_record_processor_get_field_info(
    TeccRecordProcessorContext *context,
    int field_index)
{
    if (context == NULL) {
        return NULL;
    }

    if (field_index < 0 || field_index >= context->num_fields) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid field index: %d (total fields: %d)",
                           field_index, context->num_fields);
        return NULL;
    }

    /* Lazy initialization of field info */
    if (context->field_info[field_index] == NULL) {
        context->field_info[field_index] = create_field_info(context->conn,
                                                            context->result,
                                                            field_index);
        if (context->field_info[field_index] == NULL) {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "Failed to create field info for field %d",
                               field_index);
            return NULL;
        }

        if (context->field_info[field_index]->is_encrypted) {
            context->num_encrypted_fields++;
        }
    }

    return context->field_info[field_index];
}

/* Check if a field is encrypted */
bool
tecc_record_processor_is_field_encrypted(
    TeccRecordProcessorContext *context,
    int field_index)
{
    TeccRecordFieldInfo *field_info;

    field_info = tecc_record_processor_get_field_info(context, field_index);
    if (field_info == NULL) {
        return false;
    }

    return field_info->is_encrypted;
}

/* Clean up record processor context */
void
tecc_record_processor_cleanup(TeccRecordProcessorContext *context)
{
    int i;

    if (context == NULL) {
        return;
    }

    /* Free field info array */
    if (context->field_info != NULL) {
        for (i = 0; i < context->num_fields; i++) {
            if (context->field_info[i] != NULL) {
                free_field_info(context->field_info[i]);
            }
        }
        pg_free(context->field_info);
    }

    /* Free context */
    pg_free(context);

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Record processor context cleaned up");
}

/* Unified record decryption interface for external modules */
TeccRecordProcessorResult
tecc_record_processor_deprocess(
    PGconn *conn,
    PGresult *result,
    int row_index,
    int field_index,
    char **decrypted_value)
{
    TeccRecordProcessorContext *context;
    TeccRecordProcessorResult ret;
    char **row_values;

    if (conn == NULL || result == NULL || decrypted_value == NULL) {
        return TECC_RECORD_PROCESSOR_INVALID_INPUT;
    }

    if (row_index < 0 || row_index >= PQntuples(result) ||
        field_index < 0 || field_index >= PQnfields(result)) {
        return TECC_RECORD_PROCESSOR_INVALID_INPUT;
    }

    /* Initialize context */
    context = tecc_record_processor_init(conn, result);
    if (context == NULL) {
        return TECC_RECORD_PROCESSOR_ERROR;
    }

    /* Process the specific row */
    ret = tecc_record_processor_process_row(context, row_index, &row_values);
    if (ret == TECC_RECORD_PROCESSOR_SUCCESS) {
        /* Return the decrypted value for the specific field */
        if (row_values[field_index] != NULL) {
            *decrypted_value = pg_strdup(row_values[field_index]);
            if (*decrypted_value == NULL) {
                ret = TECC_RECORD_PROCESSOR_MEMORY_ERROR;
            }
        } else {
            *decrypted_value = NULL;
        }

        /* Clean up row values */
        {
            int i;
            for (i = 0; i < context->num_fields; i++) {
                if (row_values[i] != NULL) {
                    pg_free(row_values[i]);
                }
            }
        }
        pg_free(row_values);
    }

    /* Clean up context */
    tecc_record_processor_cleanup(context);

    return ret;
}

/* Internal helper functions */

/* Create field information for a specific field */
static TeccRecordFieldInfo *
create_field_info(PGconn *conn, PGresult *result, int field_index)
{
    TeccRecordFieldInfo *field_info;
    const char *field_name;
    Oid type_oid;

    field_info = (TeccRecordFieldInfo *)pg_malloc(sizeof(TeccRecordFieldInfo));
    if (field_info == NULL) {
        return NULL;
    }

    memset(field_info, 0, sizeof(TeccRecordFieldInfo));

    /* Get basic field information */
    field_name = PQfname(result, field_index);
    type_oid = PQftype(result, field_index);

    if (field_name != NULL) {
        field_info->column_name = pg_strdup(field_name);
    }

    field_info->type_oid = type_oid;
    field_info->field_index = field_index;
    field_info->table_oid = InvalidOid;  /* Initialize to invalid OID */

    /* Check if this is an encrypted type */
    field_info->is_encrypted = is_encrypted_type_oid(type_oid);

    if (field_info->is_encrypted) {
        /* ReqID:TECC4.0 - Extract table information from result metadata */
        /* Get table OID using PQftable */
        Oid table_oid = PQftable(result, field_index);
        TeccColumnCacheItem *cache_result = NULL;

        if (table_oid != InvalidOid) {
            /* We have table OID, but need table name */
            /* For now, format as "table_<oid>" since we don't have name lookup */
            char table_name_buf[64];
            snprintf(table_name_buf, sizeof(table_name_buf), "table_%u", table_oid);
            field_info->table_name = pg_strdup(table_name_buf);

            /* Also store table OID for potential cache lookup by OID */
            field_info->table_oid = table_oid;
        } else {
            /* Table OID not available - use placeholder */
            field_info->table_name = pg_strdup("unknown");
            field_info->table_oid = InvalidOid;
        }

        /* Get encryption metadata from cache */
        /* ReqID:TECC4.0 - Implement metadata lookup */

        /* Try to get metadata using table OID if available */
        if (field_info->table_oid != InvalidOid) {
            /* Use PQftablecol to get column attribute number */
            int column_attnum = PQftablecol(result, field_index);
            if (column_attnum > 0) {
                /* 使用新的缓存接口查询加密列信息 */
                TeccState *state = tecc_get_state(conn);
                if (state != NULL) {
                    cache_result = tecc_cache_get_encrypted_column(
                        state, field_info->table_oid, (int16)column_attnum);
                }
            }
        }

        /* If not found by OID, try by name */
        if (cache_result == NULL && field_info->table_name != NULL && field_info->column_name != NULL) {
            /* Use new cache interface tecc_cache_get_encrypted_column_by_name() */
            TeccState *state = tecc_get_state(conn);
            if (state != NULL) {
                /* Get database name from connection */
                const char *database_name = PQdb(conn);
                if (database_name == NULL || database_name[0] == '\0') {
                    /* 无法获取数据库名，记录警告 */
                    tecc_processor_debug(TECC_LOG_WARNING,
                                       "create_field_info: Could not get database name from connection");
                } else {
                    /* Get current schema from cache */
                    const char *schema_name = tecc_cache_get_guc_param(state, "current_schema");
                    if (schema_name == NULL || schema_name[0] == '\0') {
                        /* 无法获取schema名，记录警告 */
                        tecc_processor_debug(TECC_LOG_WARNING,
                                           "create_field_info: Could not get current schema from cache");
                    } else {
                        /* 使用新的缓存接口查询加密列信息 */
                        cache_result = tecc_cache_get_encrypted_column_by_name(
                            state,
                            database_name,
                            schema_name,
                            field_info->table_name,
                            field_info->column_name);
                    }
                }
            }
        }

        if (cache_result != NULL) {
            /* Found metadata in cache */
            /* Resolve aliased column name to real column name */
            if (cache_result->column_name != NULL) {
                if (field_info->column_name != NULL)
                    pg_free(field_info->column_name);
                field_info->column_name = pg_strdup(cache_result->column_name);
            }
            if (cache_result->table_name != NULL) {
                if (field_info->table_name != NULL)
                    pg_free(field_info->table_name);
                field_info->table_name = pg_strdup(cache_result->table_name);
            }
            /* Determine encryption algorithm based on metadata */
            /* encryption_type位掩码: 0x01=DET, 0x02=OPE, 0x04=SEARCH */
            if (cache_result->encryption_type & 0x04) { /* SEARCH */
                field_info->encryption_algorithm = pg_strdup("SEARCH");
            } else if (cache_result->encryption_type & 0x02) { /* OPE */
                field_info->encryption_algorithm = pg_strdup("SM4-OPE");
            } else if (cache_result->encryption_type & 0x01) { /* DET */
                field_info->encryption_algorithm = pg_strdup("DET-SM4");
            } else {
                field_info->encryption_algorithm = pg_strdup("SM4-RND");
            }

            /* Get KEK and DEK names from cache */
            if (cache_result->dek_name != NULL) {
                field_info->dek_name = pg_strdup(cache_result->dek_name);
            } else {
                field_info->dek_name = pg_strdup("default_dek");
            }
            /* TODO: 需要从DEK获取KEK名称 */
            field_info->kek_name = pg_strdup("default_kek");

            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Found metadata for encrypted column %s.%s (algorithm: %s, dek: %s)",
                               field_info->table_name, field_info->column_name,
                               field_info->encryption_algorithm, field_info->dek_name);
        } else {
            /* Metadata not found in cache - use defaults */
            field_info->encryption_algorithm = pg_strdup("SM4-RND");
            field_info->kek_name = pg_strdup("default_kek");
            field_info->dek_name = pg_strdup("default_dek");

            tecc_processor_debug(TECC_LOG_WARNING,
                               "Metadata not found for encrypted column %s.%s, using defaults",
                               field_info->table_name, field_info->column_name);
        }
    }

    return field_info;
}

/* Free field information */
static void
free_field_info(TeccRecordFieldInfo *field_info)
{
    if (field_info == NULL) {
        return;
    }

    if (field_info->table_name != NULL) {
        pg_free(field_info->table_name);
    }
    if (field_info->column_name != NULL) {
        pg_free(field_info->column_name);
    }
    if (field_info->encryption_algorithm != NULL) {
        pg_free(field_info->encryption_algorithm);
    }
    if (field_info->kek_name != NULL) {
        pg_free(field_info->kek_name);
    }
    if (field_info->dek_name != NULL) {
        pg_free(field_info->dek_name);
    }

    pg_free(field_info);
}

/* Decrypt a field value - placeholder implementation */
static TeccRecordProcessorResult
decrypt_field_value(PGconn *conn,
                   TeccRecordFieldInfo *field_info,
                   const char *encrypted_value,
                   char **decrypted_value)
{
    TeccUnifiedDecryptResult *decrypt_result;

    if (field_info == NULL || encrypted_value == NULL || decrypted_value == NULL) {
        return TECC_RECORD_PROCESSOR_INVALID_INPUT;
    }

    if (!field_info->is_encrypted) {
        /* Not encrypted - just copy the value */
        *decrypted_value = pg_strdup(encrypted_value);
        if (*decrypted_value == NULL) {
            return TECC_RECORD_PROCESSOR_MEMORY_ERROR;
        }
        return TECC_RECORD_PROCESSOR_SUCCESS;
    }

    /* Implement actual decryption using crypto scheduler */
    decrypt_result = tecc_unified_decrypt_hex(conn,
                                             encrypted_value,
                                             field_info->table_name,
                                             field_info->column_name,
                                             NULL);

    if (decrypt_result == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to decrypt field %s.%s: scheduler returned NULL",
                           field_info->table_name, field_info->column_name);
        return TECC_RECORD_PROCESSOR_DECRYPTION_ERROR;
    }

    if (decrypt_result->error_code != TECC_CRYPTO_SUCCESS) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Decryption failed for field %s.%s: %s (error code: %d)",
                           field_info->table_name, field_info->column_name,
                           decrypt_result->error_msg, decrypt_result->error_code);
        tecc_unified_free_decrypt_result(decrypt_result);
        return TECC_RECORD_PROCESSOR_DECRYPTION_ERROR;
    }

    /* Copy decrypted plaintext */
    *decrypted_value = pg_strdup(decrypt_result->plaintext);
    if (*decrypted_value == NULL) {
        tecc_unified_free_decrypt_result(decrypt_result);
        return TECC_RECORD_PROCESSOR_MEMORY_ERROR;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Successfully decrypted field %s.%s (plain_len=%zu)",
                       field_info->table_name, field_info->column_name,
                       decrypt_result->plain_len);

    /* Clean up decrypt result */
    tecc_unified_free_decrypt_result(decrypt_result);

    return TECC_RECORD_PROCESSOR_SUCCESS;
}

/* Check if a type OID corresponds to an encrypted type */
static bool
is_encrypted_type_oid(Oid type_oid)
{
    /* ReqID:TECC4.0 - Implement encrypted type detection */
    /* Check if type OID matches any encrypted type or encrypted array type */
    switch (type_oid)
    {
        case INT2_ENCOID:
        case INT4_ENCOID:
        case INT8_ENCOID:
        case FLOAT4_ENCOID:
        case FLOAT8_ENCOID:
        case NUMERIC_ENCOID:
        case VARCHAR_ENCOID:
        case BPCHAR_ENCOID:
        case TEXT_ENCOID:
        case INT2_ENCARRAYOID:
        case INT4_ENCARRAYOID:
        case INT8_ENCARRAYOID:
        case FLOAT4_ENCARRAYOID:
        case FLOAT8_ENCARRAYOID:
        case NUMERIC_ENCARRAYOID:
        case VARCHAR_ENCARRAYOID:
        case BPCHAR_ENCARRAYOID:
        case TEXT_ENCARRAYOID:
            return true;
        default:
            return false;
    }
}
/* End - ReqID:TECC4.0 */