#include "postgres_fe.h"
#include "common/fe_memutils.h"
#include "common/shortest_dec.h"
#include "catalog/pg_type_d.h"     /* For FLOAT4OID, FLOAT8OID constants */
/* Begin - ReqID:TECC4.0 */
/*
 * File: raw_value.c
 *
 * Abstract:
 *   Raw value implementation for TECC processor module.
 *   Implements operations on individual raw values including
 *   creation, processing, and destruction.
 *
 * Authored by <wangxp@highgo.com>, 20260403.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/raw_value.c
 */

#include "raw_value.h"
#include "internal/tecc_processor_internal.h"  /* For debug functions */

/* Forward declarations for external module interfaces */
#include "../tecc_scheduler/tecc_crypto_scheduler.h"  /* Crypto scheduler interface */
#include "../tecc_formatter/tecc_formatter.h"  /* Data formatter interface */

/*
 * Forward declarations
 */

static char *tecc_binary_to_hex(const void *binary_data, size_t length);

/*
 * Create new raw value
 */
TeccRawValue *
tecc_raw_value_create(TeccRawValueType type)
{
    TeccRawValue *raw_value;

    raw_value = pg_malloc0(sizeof(TeccRawValue));
    if (raw_value == NULL)
        return NULL;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "tecc_raw_value_create: raw_value=%p, requested type=%d",
                        raw_value, type);
    raw_value->type = type;
    raw_value->state = TECC_RAW_VALUE_STATE_ORIGINAL;
    raw_value->needs_encryption = false;
    raw_value->needs_decryption = false;
    raw_value->is_null = false;
    raw_value->is_default = false;  /* ReqID:TECC4.0 */
    raw_value->length = 0;
    raw_value->column_name = NULL;
    raw_value->column_type = NULL;
    raw_value->type_oid = InvalidOid;

    /* Initialize SQL rewrite support fields */
    raw_value->is_param = false;
    raw_value->param_index = -1;
    raw_value->location = 0;
    raw_value->data_size = 0;
    raw_value->position_offset = 0;
    raw_value->data_value_format = 0;
    raw_value->data_value = NULL;
    raw_value->data_value_size = 0;
    raw_value->processed_data_needs_free = false;

    return raw_value;
}

/*
 * Create raw value from string
 */
TeccRawValue *
tecc_raw_value_create_from_string(const char *value, size_t length)
{
    TeccRawValue *raw_value;

    raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_STRING);
    if (raw_value == NULL)
        return NULL;

    if (value != NULL)
    {
        raw_value->original.string_value = pg_strdup(value);
        raw_value->length = (length > 0) ? length : strlen(value);
    }
    else
    {
        raw_value->is_null = true;
        raw_value->length = 0;
    }

    return raw_value;
}

/*
 * Create raw value from integer
 */
TeccRawValue *
tecc_raw_value_create_from_int(int64_t value)
{
    TeccRawValue *raw_value;

    raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_INTEGER);
    if (raw_value == NULL)
        return NULL;

    raw_value->original.integer_value = value;
    raw_value->length = sizeof(int64_t);

    return raw_value;
}

/*
 * Create raw value from float
 */
TeccRawValue *
tecc_raw_value_create_from_float(double value)
{
    TeccRawValue *raw_value;

    raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_FLOAT);
    if (raw_value == NULL)
        return NULL;

    raw_value->original.float_value = value;
    raw_value->length = sizeof(double);

    return raw_value;
}

/*
 * Create raw value from binary data
 */
TeccRawValue *
tecc_raw_value_create_from_binary(const void *data, size_t length)
{
    TeccRawValue *raw_value;

    raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_BINARY);
    if (raw_value == NULL)
        return NULL;

    if (data != NULL && length > 0)
    {
        raw_value->original.binary_value = pg_malloc(length);
        if (raw_value->original.binary_value == NULL)
        {
            pg_free(raw_value);
            return NULL;
        }
        memcpy(raw_value->original.binary_value, data, length);
        raw_value->length = length;
    }
    else
    {
        raw_value->is_null = true;
        raw_value->length = 0;
    }

    return raw_value;
}

/*
 * Create NULL raw value
 */
TeccRawValue *
tecc_raw_value_create_null(void)
{
    TeccRawValue *raw_value;

    raw_value = tecc_raw_value_create(TECC_RAW_VALUE_TYPE_UNKNOWN);
    if (raw_value == NULL)
        return NULL;

    raw_value->is_null = true;
    raw_value->length = 0;

    return raw_value;
}

/*
 * Copy raw value
 */
TeccRawValue *
tecc_raw_value_copy(const TeccRawValue *source)
{
    TeccRawValue *copy;

    if (source == NULL)
        return NULL;

    copy = tecc_raw_value_create(source->type);
    if (copy == NULL)
        return NULL;

    /* Copy basic fields */
    copy->state = source->state;
    copy->needs_encryption = source->needs_encryption;
    copy->needs_decryption = source->needs_decryption;
    copy->is_null = source->is_null;
    copy->is_default = source->is_default;  /* ReqID:TECC4.0 */
    copy->is_sql_expression = source->is_sql_expression; /* ReqID:TECC4.0 */
    copy->length = source->length;
    copy->type_oid = source->type_oid;
    copy->location = source->location;       /* ReqID:TECC4.0 - Bug 9152: needed for SQL rewrite */
    copy->data_size = source->data_size;     /* ReqID:TECC4.0 - Bug 9152: needed for SQL rewrite */

    /* Copy column info */
    if (source->column_name != NULL)
        copy->column_name = pg_strdup(source->column_name);
    if (source->column_type != NULL)
        copy->column_type = pg_strdup(source->column_type);

    /* Copy table info */
    if (source->schema_name != NULL)
        copy->schema_name = pg_strdup(source->schema_name);
    if (source->table_name != NULL)
        copy->table_name = pg_strdup(source->table_name);

    /* Copy original value based on type */
    if (!copy->is_null)
    {
        switch (source->type)
        {
            case TECC_RAW_VALUE_TYPE_STRING:
                if (source->original.string_value != NULL)
                    copy->original.string_value = pg_strdup(source->original.string_value);
                break;

            case TECC_RAW_VALUE_TYPE_INTEGER:
                copy->original.integer_value = source->original.integer_value;
                break;

            case TECC_RAW_VALUE_TYPE_FLOAT:
                copy->original.float_value = source->original.float_value;
                break;

            case TECC_RAW_VALUE_TYPE_BINARY:
                if (source->original.binary_value != NULL && source->length > 0)
                {
                    copy->original.binary_value = pg_malloc(source->length);
                    if (copy->original.binary_value != NULL)
                        memcpy(copy->original.binary_value, source->original.binary_value, source->length);
                }
                break;

            default:
                break;
        }
    }

    /* Copy processed value if exists */
    if (source->state != TECC_RAW_VALUE_STATE_ORIGINAL)
    {
        switch (source->state)
        {
            case TECC_RAW_VALUE_STATE_ENCRYPTED:
            case TECC_RAW_VALUE_STATE_PROCESSED: /* ReqID:TECC4.0 - Bug 9152: PROCESSED also uses encrypted_value */
                if (source->processed.encrypted_value != NULL)
                    copy->processed.encrypted_value = pg_strdup(source->processed.encrypted_value);
                break;

            case TECC_RAW_VALUE_STATE_DECRYPTED:
                if (source->processed.decrypted_value != NULL)
                    copy->processed.decrypted_value = pg_strdup(source->processed.decrypted_value);
                break;

            default:
                break;
        }
    }

    return copy;
}

/*
 * Process raw value (prepare for encryption/decryption)
 */
int
tecc_raw_value_process(TeccRawValue *raw_value)
{
    if (raw_value == NULL)
        return -1;

    /* Skip if already processed or null */
    if (raw_value->state != TECC_RAW_VALUE_STATE_ORIGINAL || raw_value->is_null || raw_value->is_default)
        return 0;

    /* Note: needs_encryption flag should be set by the caller before calling this function */
    /* This function only prepares the value for encryption/decryption, does not perform encryption */

    /* If encryption is needed, prepare value for encryption */
    if (raw_value->needs_encryption && raw_value->column_name != NULL)
    {
        /* Convert value to string for encryption */
        switch (raw_value->type)
        {
            case TECC_RAW_VALUE_TYPE_STRING:
                break;
            case TECC_RAW_VALUE_TYPE_INTEGER:
                {
                    char temp[32];
                    snprintf(temp, sizeof(temp), "%lld", (long long) raw_value->original.integer_value);
                    raw_value->original.string_value = pg_strdup(temp);
                    raw_value->type = TECC_RAW_VALUE_TYPE_STRING;
                    break;
                }
            case TECC_RAW_VALUE_TYPE_FLOAT:
                {
                    char temp[64];
                    /* ReqID:TECC4.0 - Use PostgreSQL backend's Ryu shortest-decimal
                     * (float4out/float8out) for type-distinguished normalization.
                     * Aligns with backend float4out/float8out behavior:
                     * float4(1.1f) → "1.1", float8(1.23) → "1.23". */
                    if (raw_value->type_oid == FLOAT4OID)
                    {
                        float f = (float)raw_value->original.float_value;
                        float_to_shortest_decimal_buf(f, temp);
                    }
                    else if (raw_value->type_oid == FLOAT8OID)
                    {
                        double_to_shortest_decimal_buf(raw_value->original.float_value, temp);
                    }
                    else
                    {
                        /* NUMERIC or unknown — use full float8 Ryu */
                        double_to_shortest_decimal_buf(raw_value->original.float_value, temp);
                    }
                    raw_value->original.string_value = pg_strdup(temp);
                    raw_value->type = TECC_RAW_VALUE_TYPE_STRING;
                    break;
                }
            case TECC_RAW_VALUE_TYPE_BINARY:
                /* For binary data, we need to handle it based on the actual data type */
                /* TECC只支持9种加密类型，没有二进制类型。二进制指的是数据格式，不是数据类型 */
                if (raw_value->original.binary_value != NULL && raw_value->length > 0)
                {
                    /* For binary format data, we need to convert it to appropriate format */
                    /* The actual handling depends on the column type */
                    if (raw_value->column_type != NULL)
                    {
                        /* Check if this is a numeric type that needs special handling */
                        if (strstr(raw_value->column_type, "int") != NULL ||
                            strstr(raw_value->column_type, "float") != NULL ||
                            strstr(raw_value->column_type, "numeric") != NULL)
                        {
                            /* Numeric types in binary format need conversion to string */
                            /* This is a simplified implementation - actual conversion depends on type OID */
                            char temp[64];
                            if (raw_value->length == sizeof(int32))
                            {
                                int32_t int_val;
                                memcpy(&int_val, raw_value->original.binary_value, sizeof(int32_t));
                                snprintf(temp, sizeof(temp), "%d", int_val);
                            }
                            else if (raw_value->length == sizeof(int64))
                            {
                                int64_t int_val;
                                memcpy(&int_val, raw_value->original.binary_value, sizeof(int64_t));
                                snprintf(temp, sizeof(temp), "%ld", (long)int_val);
                            }
                            else if (raw_value->length == sizeof(float))
                            {
                                float float_val;
                                memcpy(&float_val, raw_value->original.binary_value, sizeof(float));
                                snprintf(temp, sizeof(temp), "%f", float_val);
                            }
                            else if (raw_value->length == sizeof(double))
                            {
                                double double_val;
                                memcpy(&double_val, raw_value->original.binary_value, sizeof(double));
                                snprintf(temp, sizeof(temp), "%f", double_val);
                            }
                            else
                            {
                                /* Generic binary to hex for other types */
                                char *hex_str = tecc_binary_to_hex(raw_value->original.binary_value, raw_value->length);
                                if (hex_str != NULL)
                                {
                                    raw_value->original.string_value = hex_str;
                                }
                                break;
                            }
                            raw_value->original.string_value = pg_strdup(temp);
                        }
                        else
                        {
                            /* Text types in binary format - treat as raw bytes */
                            /* Convert binary to hex string for encryption */
                            char *hex_str = tecc_binary_to_hex(raw_value->original.binary_value, raw_value->length);
                            if (hex_str != NULL)
                            {
                                raw_value->original.string_value = hex_str;
                            }
                        }
                    }
                    else
                    {
                        /* No column type info - treat as generic binary data */
                        char *hex_str = tecc_binary_to_hex(raw_value->original.binary_value, raw_value->length);
                        if (hex_str != NULL)
                        {
                            raw_value->original.string_value = hex_str;
                        }
                    }
                }
                break;
            default:
                break;
        }

        /* Note: Actual encryption is performed by tecc_values_processor_encrypt_value function */
        /* This function only prepares the value (converts to string) for encryption */
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Value prepared for encryption: column %s",
                           raw_value->column_name ? raw_value->column_name : "unknown");
    }

    raw_value->state = TECC_RAW_VALUE_STATE_PROCESSED;
    return 0;
}

/*
 * Set column information
 */
void
tecc_raw_value_set_column_info(TeccRawValue *raw_value,
                              const char *column_name,
                              const char *column_type,
                              Oid type_oid)
{
    if (raw_value == NULL)
        return;

    if (column_name != NULL)
    {
        if (raw_value->column_name != NULL)
            pg_free(raw_value->column_name);
        raw_value->column_name = pg_strdup(column_name);
    }

    if (column_type != NULL)
    {
        if (raw_value->column_type != NULL)
            pg_free(raw_value->column_type);
        raw_value->column_type = pg_strdup(column_type);
    }

    raw_value->type_oid = type_oid;
}

/*
 * Set table information
 */
void
tecc_raw_value_set_table_info(TeccRawValue *raw_value,
                             const char *schema_name,
                             const char *table_name)
{
    if (raw_value == NULL)
        return;

    if (schema_name != NULL)
    {
        if (raw_value->schema_name != NULL)
            pg_free(raw_value->schema_name);
        raw_value->schema_name = pg_strdup(schema_name);
    }

    if (table_name != NULL)
    {
        if (raw_value->table_name != NULL)
            pg_free(raw_value->table_name);
        raw_value->table_name = pg_strdup(table_name);
    }
}

/*
 * Destroy raw value
 */
void
tecc_raw_value_destroy(TeccRawValue *raw_value)
{
    if (raw_value == NULL)
        return;

    /* Free column info */
    if (raw_value->column_name != NULL)
        pg_free(raw_value->column_name);
    if (raw_value->column_type != NULL)
        pg_free(raw_value->column_type);

    /* Free table info */
    if (raw_value->schema_name != NULL)
        pg_free(raw_value->schema_name);
    if (raw_value->table_name != NULL)
        pg_free(raw_value->table_name);

    /* Free original value based on type */
    if (!raw_value->is_null)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "tecc_raw_value_destroy: raw_value=%p, type=%d, is_null=%d",
                            raw_value, raw_value->type, raw_value->is_null);
        switch (raw_value->type)
        {
            case TECC_RAW_VALUE_TYPE_STRING:
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                    "Destroying STRING value: string_value=%p",
                                    raw_value->original.string_value);
                if (raw_value->original.string_value != NULL)
                    pg_free(raw_value->original.string_value);
                break;

            case TECC_RAW_VALUE_TYPE_BINARY:
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                    "Destroying BINARY value: binary_value=%p",
                                    raw_value->original.binary_value);
                if (raw_value->original.binary_value != NULL)
                    pg_free(raw_value->original.binary_value);
                break;

            case TECC_RAW_VALUE_TYPE_FLOAT:
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                    "Destroying FLOAT value: float_value=%f",
                                    raw_value->original.float_value);
                /* FLOAT type doesn't need freeing - it's stored directly in the union */
                break;

            case TECC_RAW_VALUE_TYPE_INTEGER:
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                    "Destroying INTEGER value: integer_value=%lld",
                                    (long long) raw_value->original.integer_value);
                /* INTEGER type doesn't need freeing - it's stored directly in the union */
                break;

            default:
                tecc_processor_debug(TECC_LOG_WARNING,
                                    "Unknown type %d in tecc_raw_value_destroy",
                                    raw_value->type);
                break;
        }
    }

    /* Free processed value based on state */
    switch (raw_value->state)
    {
        case TECC_RAW_VALUE_STATE_ENCRYPTED:
            if (raw_value->processed.encrypted_value != NULL)
                pg_free(raw_value->processed.encrypted_value);
            break;

        case TECC_RAW_VALUE_STATE_DECRYPTED:
            if (raw_value->processed.decrypted_value != NULL)
                pg_free(raw_value->processed.decrypted_value);
            break;

        default:
            break;
    }

    pg_free(raw_value);
}

/*
 * Get string representation
 */
char *
tecc_raw_value_to_string(const TeccRawValue *raw_value)
{
    char *result;
    size_t len;

    if (raw_value == NULL)
        return NULL;

    if (raw_value->is_null)
    {
        result = pg_malloc(5);
        strcpy(result, "NULL");
        return result;
    }

    switch (raw_value->state)
    {
        case TECC_RAW_VALUE_STATE_ENCRYPTED:
            if (raw_value->processed.encrypted_value != NULL)
                return pg_strdup(raw_value->processed.encrypted_value);
            break;

        case TECC_RAW_VALUE_STATE_DECRYPTED:
            if (raw_value->processed.decrypted_value != NULL)
                return pg_strdup(raw_value->processed.decrypted_value);
            break;

        default:
            /* Use original value */
            break;
    }

    /* Convert original value to string based on type */
    switch (raw_value->type)
    {
        case TECC_RAW_VALUE_TYPE_STRING:
            if (raw_value->original.string_value != NULL)
                return pg_strdup(raw_value->original.string_value);
            break;

        case TECC_RAW_VALUE_TYPE_INTEGER:
            len = 32;
            result = pg_malloc(len);
            snprintf(result, len, "%lld", (long long) raw_value->original.integer_value);
            return result;

        case TECC_RAW_VALUE_TYPE_FLOAT:
            /* ReqID:TECC4.0 - Use float4/8 Ryū shortest decimal to preserve
             * float mantissa per design: string→float→normalize. Check length
             * to distinguish float4 (mantissa includes float4→double promotion)
             * from float8, matching the COPY path at lines 310-323. */
            if (raw_value->length == sizeof(float))
            {
                result = pg_malloc(FLOAT_SHORTEST_DECIMAL_LEN);
                float_to_shortest_decimal_buf((float)raw_value->original.float_value, result);
            }
            else
            {
                result = pg_malloc(DOUBLE_SHORTEST_DECIMAL_LEN);
                double_to_shortest_decimal_buf(raw_value->original.float_value, result);
            }
            return result;

        case TECC_RAW_VALUE_TYPE_BINARY:
            /* Binary data - return hex representation or placeholder */
            result = pg_malloc(32);
            snprintf(result, 32, "<binary:%zu>", raw_value->length);
            return result;

        default:
            break;
    }

    return NULL;
}

/*
 * Check if value is null
 */
bool
tecc_raw_value_is_null(const TeccRawValue *raw_value)
{
    if (raw_value == NULL)
        return true;

    return raw_value->is_null;
}

/*
 * Convert binary data to hex string
 * Helper function for binary format data processing
 */
static char *
tecc_binary_to_hex(const void *binary_data, size_t length)
{
    static const char hex_chars[] = "0123456789abcdef";
    char *hex_str;
    size_t i;

    if (binary_data == NULL || length == 0)
        return NULL;

    /* Allocate memory for hex string (2 chars per byte + null terminator) */
    hex_str = pg_malloc(length * 2 + 1);
    if (hex_str == NULL)
        return NULL;

    /* Convert each byte to two hex characters */
    for (i = 0; i < length; i++)
    {
        unsigned char byte = ((const unsigned char *)binary_data)[i];
        hex_str[i * 2] = hex_chars[(byte >> 4) & 0x0F];
        hex_str[i * 2 + 1] = hex_chars[byte & 0x0F];
    }

    hex_str[length * 2] = '\0';
    return hex_str;
}

/* End - ReqID:TECC4.0 */
