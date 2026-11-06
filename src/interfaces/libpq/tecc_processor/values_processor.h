/* Begin - ReqID:TECC4.0 */
/*
 * File: values_processor.h
 *
 * Abstract:
 *   Values processor submodule for TECC processor module.
 *   Handles encryption/decryption of values in SQL statements
 *   and manages raw value lists for batch processing.
 *
 * Authored by <wangxp@highgo.com>, 20260403.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/values_processor.h
 */

#ifndef VALUES_PROCESSOR_H
#define VALUES_PROCESSOR_H

#include "tecc_processor.h"
#include "internal/tecc_processor_internal.h"
#include "raw_value.h"
#include "raw_values_list.h"

/*
 * Values processor result codes
 */
typedef enum TeccValuesProcessorResult
{
    TECC_VALUES_PROCESSOR_SUCCESS = 0,
    TECC_VALUES_PROCESSOR_ERROR = -1,
    TECC_VALUES_PROCESSOR_MEMORY_ERROR = -2,
    TECC_VALUES_PROCESSOR_ENCRYPTION_ERROR = -3,
    TECC_VALUES_PROCESSOR_DECRYPTION_ERROR = -4,
    TECC_VALUES_PROCESSOR_TYPE_MISMATCH = -5
} TeccValuesProcessorResult;

/*
 * Decrypt result codes (参考openGauss设计)
 */
typedef enum TeccDecryptResult
{
    TECC_DECRYPT_SUCCESS = 0,               /* 解密成功 */
    TECC_DECRYPT_ERROR,                     /* 解密错误 */
    TECC_INVALID_DATA_LENGTH,               /* 无效数据长度 */
    TECC_CEK_DECRYPT_ERROR,                 /* CEK解密错误 */
    TECC_CEK_DERIVE_ERROR,                  /* CEK派生错误 */
    TECC_MEMORY_ERROR,                      /* 内存错误 */
    TECC_CACHE_ERROR,                       /* 缓存错误 */
    TECC_FORMAT_ERROR                       /* 格式错误 */
} TeccDecryptResult;

/*
 * Process status enumeration (参考openGauss设计)
 */
typedef enum TeccProcessStatus
{
    TECC_ONLY_VALUE = 0,                    /* 仅值本身，如：text */
    TECC_ADD_QUOTATION,                     /* 添加引号，如：'text' */
    TECC_ADD_TYPE                           /* 添加类型，如：'text'::varchar */
} TeccProcessStatus;

/*
 * Values processing context
 */
typedef struct TeccValuesProcessorContext
{
    TeccRawValuesList *raw_values_list;
    List *encrypted_columns;
    int encryption_algorithm;
    char *encryption_key;
    bool needs_processing;
} TeccValuesProcessorContext;

/*
 * Values processor interface functions
 */

/* Main value processing entry point */
TeccValuesProcessorResult tecc_values_processor_process(TeccStatementData *statement_data);

/* Get encrypted type name from base type OID */
const char *get_enc_type_name_from_base_oid(Oid type_oid);

/* Main value deprocessing entry point */
TeccValuesProcessorResult tecc_values_processor_deprocess(TeccStatementData *statement_data);

/* Process values for INSERT statement */
TeccValuesProcessorResult tecc_values_processor_process_insert_values(List *values_lists,
                                                                     List *target_columns,
                                                                     const char *schema_name,
                                                                     const char *table_name,
                                                                     TeccStatementData *statement_data);

/* Process values for UPDATE statement */
TeccValuesProcessorResult tecc_values_processor_process_update_values(List *target_list,
                                                                     List *target_columns,
                                                                     const char *schema_name,
                                                                     const char *table_name,
                                                                     TeccStatementData *statement_data);

/* Process individual value */
TeccValuesProcessorResult tecc_values_processor_process_value(TeccRawValue *raw_value,
                                                             const char *column_name,
                                                             const char *column_type,
                                                             TeccStatementData *statement_data);

/* Encrypt single value */
TeccValuesProcessorResult tecc_values_processor_encrypt_value(TeccRawValue *raw_value,
                                                             const char *algorithm,
                                                             const char *key,
                                                             TeccStatementData *statement_data);

/* Decrypt single value */
TeccValuesProcessorResult tecc_values_processor_decrypt_value(TeccRawValue *raw_value,
                                                             const char *algorithm,
                                                             const char *key,
                                                             TeccStatementData *statement_data);

/* Get algorithm for column with statement data */
const char *tecc_values_processor_get_column_algorithm_with_state(TeccStatementData *statement_data,
                                                                 const char *schema_name,
                                                                 const char *table_name,
                                                                 const char *column_name,
                                                                 const char *operator_name);

/* Get encryption key for column with statement data */
const char *tecc_values_processor_get_column_key_with_state(TeccStatementData *statement_data,
                                                           const char *schema_name,
                                                           const char *table_name,
                                                           const char *column_name);

/* Process query parameters for encryption */
TeccValuesProcessorResult tecc_values_processor_process_parameters(TeccStatementData *statement_data);

/* Rewrite values in SQL query */
TeccValuesProcessorResult tecc_values_processor_rewrite_values(TeccStatementData *statement_data);

/* Decrypt and restore value (参考openGauss设计) */
TeccDecryptResult tecc_values_processor_deprocess_value(PGconn *conn,
                                                       const unsigned char *processed_data,
                                                       size_t processed_data_size,
                                                       int original_typeid,
                                                       int format,
                                                       unsigned char **plain_text,
                                                       size_t *plain_text_size,
                                                       TeccProcessStatus process_status,
                                                       const char *table_name,
                                                       const char *column_name);

/* Error handling functions */
void tecc_values_processor_set_error(TeccStatementData *statement_data,
                                    int error_code,
                                    const char *format, ...);

const char *tecc_values_processor_get_error(TeccStatementData *statement_data);

/* Context management functions */
TeccValuesProcessorContext *tecc_values_processor_context_create(void);
void tecc_values_processor_context_destroy(TeccValuesProcessorContext *context);

/* Helper functions */
TeccRawValue *tecc_create_raw_value_from_node(Node *value_node);
bool tecc_column_needs_encryption(TeccStatementData *statement_data,
                                 const char *schema_name,
                                 const char *table_name,
                                 const char *column_name);
char *tecc_get_column_dek_name(TeccStatementData *statement_data,
                              const char *schema_name,
                              const char *table_name,
                              const char *column_name);
TeccValuesProcessorResult tecc_process_values_list(TeccRawValuesList *list,
                                                  TeccStatementData *statement_data);

#endif /* VALUES_PROCESSOR_H */
/* End - ReqID:TECC4.0 */