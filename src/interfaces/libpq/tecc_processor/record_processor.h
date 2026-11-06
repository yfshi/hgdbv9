/* Begin - ReqID:TECC4.0 */
/*
 * File: record_processor.h
 *
 * Abstract:
 *   TECC record processor for query result decryption.
 *   Handles decryption of encrypted data in query result records.
 *
 * Authored by <wangxp@highgo.com>, 20260410.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/record_processor.h
 */

#ifndef TECC_RECORD_PROCESSOR_H
#define TECC_RECORD_PROCESSOR_H

#include "tecc_processor_types.h"

/* Include PostgreSQL frontend headers */
#include "postgres_fe.h"
/* Include libpq for database connection types */
#include "../libpq-fe.h"

/* Include external module interfaces */
/* Forward declarations for scheduler types */
typedef struct TeccCryptoScheduler TeccCryptoScheduler;
typedef struct TeccUnifiedDecryptResult TeccUnifiedDecryptResult;
#include "../tecc_formatter/tecc_formatter.h"

/*
 * Record decryption result codes
 */
typedef enum TeccRecordProcessorResult {
    TECC_RECORD_PROCESSOR_SUCCESS = 0,
    TECC_RECORD_PROCESSOR_ERROR,
    TECC_RECORD_PROCESSOR_NO_ENCRYPTED_COLUMNS,
    TECC_RECORD_PROCESSOR_DECRYPTION_ERROR,
    TECC_RECORD_PROCESSOR_FORMAT_ERROR,
    TECC_RECORD_PROCESSOR_MEMORY_ERROR,
    TECC_RECORD_PROCESSOR_INVALID_INPUT
} TeccRecordProcessorResult;

/*
 * Record field information structure
 */
typedef struct TeccRecordFieldInfo {
    char *table_name;          /* Table name */
    Oid table_oid;             /* Table OID */
    char *column_name;         /* Column name */
    Oid type_oid;              /* Column type OID */
    bool is_encrypted;         /* Whether column is encrypted */
    char *encryption_algorithm; /* Encryption algorithm name */
    char *kek_name;            /* KEK name */
    char *dek_name;            /* DEK name */
    int field_index;           /* Field index in result */
} TeccRecordFieldInfo;

/*
 * Record processing context
 */
typedef struct TeccRecordProcessorContext {
    PGconn *conn;              /* Database connection */
    PGresult *result;          /* Query result */
    TeccRecordFieldInfo **field_info; /* Array of field information */
    int num_fields;            /* Number of fields */
    int num_encrypted_fields;  /* Number of encrypted fields */
    int current_row;           /* Current row being processed */
    int error_code;            /* Error code if processing failed */
    char error_msg[256];       /* Error message if processing failed */
} TeccRecordProcessorContext;

/*
 * Public interface functions
 */

/* Initialize record processor context */
TeccRecordProcessorContext *tecc_record_processor_init(PGconn *conn, PGresult *result);

/* Process a single record (row) */
TeccRecordProcessorResult tecc_record_processor_process_row(
    TeccRecordProcessorContext *context,
    int row_index,
    char ***decrypted_values);

/* Process multiple records in batch */
TeccRecordProcessorResult tecc_record_processor_process_batch(
    TeccRecordProcessorContext *context,
    int start_row,
    int num_rows,
    char ****decrypted_values);

/* Get field information for a specific column */
TeccRecordFieldInfo *tecc_record_processor_get_field_info(
    TeccRecordProcessorContext *context,
    int field_index);

/* Check if a field is encrypted */
bool tecc_record_processor_is_field_encrypted(
    TeccRecordProcessorContext *context,
    int field_index);

/* Clean up record processor context */
void tecc_record_processor_cleanup(TeccRecordProcessorContext *context);

/* Unified record decryption interface for external modules */
TeccRecordProcessorResult tecc_record_processor_deprocess(
    PGconn *conn,
    PGresult *result,
    int row_index,
    int field_index,
    char **decrypted_value);

#endif /* TECC_RECORD_PROCESSOR_H */
/* End - ReqID:TECC4.0 */