/* Begin - ReqID:TECC4.0 */
/*
 * File: raw_value.h
 *
 * Abstract:
 *   Raw value data structure and operations for TECC processor module.
 *   Represents a single value that may need encryption/decryption processing.
 *
 * Authored by <wangxp@highgo.com>, 20260403.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/raw_value.h
 */

#ifndef RAW_VALUE_H
#define RAW_VALUE_H

#include "tecc_processor_types.h"  /* Include type definitions */

/* Standard C headers for frontend code */
#include <stdint.h>  /* For int32_t */

/* int32 type for frontend code */
#ifndef int32
typedef int32_t int32;
#endif

/*
 * Raw value interface functions
 */

/* Create new raw value */
TeccRawValue *tecc_raw_value_create(TeccRawValueType type);

/* Create raw value from string */
TeccRawValue *tecc_raw_value_create_from_string(const char *value,
                                               size_t length);

/* Create raw value from integer */
TeccRawValue *tecc_raw_value_create_from_int(int64_t value);

/* Create raw value from float */
TeccRawValue *tecc_raw_value_create_from_float(double value);

/* Create raw value from binary data */
TeccRawValue *tecc_raw_value_create_from_binary(const void *data,
                                               size_t length);

/* Create NULL raw value */
TeccRawValue *tecc_raw_value_create_null(void);

/* Copy raw value */
TeccRawValue *tecc_raw_value_copy(const TeccRawValue *source);

/* Process raw value (prepare for encryption/decryption) */
int tecc_raw_value_process(TeccRawValue *raw_value);

/* Set column information */
void tecc_raw_value_set_column_info(TeccRawValue *raw_value,
                                   const char *column_name,
                                   const char *column_type,
                                   Oid type_oid);

/* Set table information */
void tecc_raw_value_set_table_info(TeccRawValue *raw_value,
                                  const char *schema_name,
                                  const char *table_name);

/* Destroy raw value */
void tecc_raw_value_destroy(TeccRawValue *raw_value);

/* Get string representation */
char *tecc_raw_value_to_string(const TeccRawValue *raw_value);

/* Check if value is null */
bool tecc_raw_value_is_null(const TeccRawValue *raw_value);

#endif /* RAW_VALUE_H */
/* End - ReqID:TECC4.0 */