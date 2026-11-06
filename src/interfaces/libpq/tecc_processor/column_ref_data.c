#include "postgres_fe.h"
#include "common/fe_memutils.h"
/* Begin - ReqID:TECC4.0 */
/*
 * File: column_ref_data.c
 *
 * Abstract:
 *   Column reference data management implementation for TECC expression processor module.
 *   Implements functions for creating, initializing, and managing TeccColumnRefData
 *   structures used in SQL expression processing.
 *
 * Authored by <wangxp@highgo.com>, 20260404.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_expression/column_ref_data.c
 */

#include "column_ref_data.h"
#include <string.h>
#include <stdio.h>

/* Include internal header for debug macro */
#include "internal/tecc_processor_internal.h"

/* Define debug level constants if not already defined */
#ifndef TECC_LOG_ERROR
#define TECC_LOG_ERROR   0
#endif

#ifndef InvalidOid
#define InvalidOid 0
#endif

/* Forward declarations for internal helper functions */
static void tecc_concat_col_fqdn(TeccColumnRefData *data);

/* Helper macro for safe string copying */
#define TECC_STRNCPY(dst, src) \
    do { \
        if ((src) != NULL && (src)[0] != '\0') { \
            strncpy((dst), (src), NAMEDATALEN - 1); \
            (dst)[NAMEDATALEN - 1] = '\0'; \
        } else { \
            (dst)[0] = '\0'; \
        } \
    } while (0)

/*
 * Create a new TeccColumnRefData object
 */
TeccColumnRefData *
tecc_column_ref_data_create(void)
{
    TeccColumnRefData *data = pg_malloc0(sizeof(TeccColumnRefData));
    if (data == NULL)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to allocate memory for TeccColumnRefData");
        return NULL;
    }

    /* Initialize fields */
    data->is_encrypted = false;
    data->column_oid = InvalidOid;
    data->table_oid = InvalidOid;
    data->location = -1;
    memset(data->alias_fqdn, 0, sizeof(data->alias_fqdn));

    return data;
}

/*
 * Destroy a TeccColumnRefData object
 */
void
tecc_column_ref_data_destroy(TeccColumnRefData *data)
{
    if (data != NULL)
    {
        pg_free(data);
    }
}

/*
 * Initialize a TeccColumnRefData object with names
 */
void
tecc_column_ref_data_init(TeccColumnRefData *data,
                         const char *catalog,
                         const char *schema,
                         const char *table,
                         const char *column)
{
    if (data == NULL)
        return;

    /* Copy names using safe string operations */
    TECC_STRNCPY(data->catalog_name, catalog);
    TECC_STRNCPY(data->schema_name, schema);
    TECC_STRNCPY(data->table_name, table);
    TECC_STRNCPY(data->column_name, column);
    /* ReqID:TECC4.0 - table_alias defaults to table_name, overridable via set_alias */
    TECC_STRNCPY(data->table_alias, table);

    /* Construct fully qualified domain name */
    tecc_concat_col_fqdn(data);
}

/* ReqID:TECC4.0 - Set table alias for multi-table UPDATE/DELETE FROM/USING scenarios.
 * When a table is referenced with an alias (e.g., FROM t2 t2_alias),
 * this overrides the default alias (which equals table_name). */
void
tecc_column_ref_data_set_alias(TeccColumnRefData *data, const char *alias_name)
{
    if (data != NULL && alias_name != NULL)
        TECC_STRNCPY(data->table_alias, alias_name);
}

/*
 * Check if column reference is fully qualified
 */
bool
tecc_column_ref_data_is_fqdn(const TeccColumnRefData *data)
{
    if (data == NULL)
        return false;

    /* Check if all name components are present */
    return (data->catalog_name[0] != '\0' &&
            data->schema_name[0] != '\0' &&
            data->table_name[0] != '\0' &&
            data->column_name[0] != '\0');
}

/*
 * Get the fully qualified domain name
 */
const char *
tecc_column_ref_data_get_fqdn(const TeccColumnRefData *data)
{
    if (data == NULL)
        return NULL;

    return data->alias_fqdn;
}

/*
 * Check if column is encrypted
 */
bool
tecc_column_ref_data_is_encrypted(const TeccColumnRefData *data)
{
    if (data == NULL)
        return false;

    return data->is_encrypted;
}

/*
 * Set column encryption status
 */
void
tecc_column_ref_data_set_encrypted(TeccColumnRefData *data, bool encrypted)
{
    if (data != NULL)
    {
        data->is_encrypted = encrypted;
    }
}

/*
 * Get column OID
 */
Oid
tecc_column_ref_data_get_column_oid(const TeccColumnRefData *data)
{
    if (data == NULL)
        return InvalidOid;

    return data->column_oid;
}

/*
 * Set column OID
 */
void
tecc_column_ref_data_set_column_oid(TeccColumnRefData *data, Oid column_oid)
{
    if (data != NULL)
    {
        data->column_oid = column_oid;
    }
}

/*
 * Get table OID
 */
Oid
tecc_column_ref_data_get_table_oid(const TeccColumnRefData *data)
{
    if (data == NULL)
        return InvalidOid;

    return data->table_oid;
}

/*
 * Set table OID
 */
void
tecc_column_ref_data_set_table_oid(TeccColumnRefData *data, Oid table_oid)
{
    if (data != NULL)
    {
        data->table_oid = table_oid;
    }
}

/*
 * Get location in SQL
 */
int
tecc_column_ref_data_get_location(const TeccColumnRefData *data)
{
    if (data == NULL)
        return -1;

    return data->location;
}

/*
 * Set location in SQL
 */
void
tecc_column_ref_data_set_location(TeccColumnRefData *data, int location)
{
    if (data != NULL)
    {
        data->location = location;
    }
}

/*
 * Get DEK name
 */
const char *
tecc_column_ref_data_get_dek_name(const TeccColumnRefData *data)
{
    if (data == NULL)
        return NULL;

    return data->dek_name;
}

/*
 * Set DEK name
 */
void
tecc_column_ref_data_set_dek_name(TeccColumnRefData *data, const char *dek_name)
{
    if (data != NULL)
    {
        TECC_STRNCPY(data->dek_name, dek_name);
    }
}

/*
 * Get supported algorithms
 */
const char *
tecc_column_ref_data_get_support_algo(const TeccColumnRefData *data)
{
    if (data == NULL)
        return NULL;

    return data->support_algo;
}

/*
 * Set supported algorithms
 */
void
tecc_column_ref_data_set_support_algo(TeccColumnRefData *data, const char *support_algo)
{
    if (data != NULL)
    {
        TECC_STRNCPY(data->support_algo, support_algo);
    }
}

/*
 * Copy column reference data
 */
TeccColumnRefData *
tecc_column_ref_data_copy(const TeccColumnRefData *src)
{
    TeccColumnRefData *dst;

    if (src == NULL)
        return NULL;

    dst = tecc_column_ref_data_create();
    if (dst == NULL)
        return NULL;

    /* Copy all fields */
    memcpy(dst->catalog_name, src->catalog_name, sizeof(dst->catalog_name));
    memcpy(dst->schema_name, src->schema_name, sizeof(dst->schema_name));
    memcpy(dst->table_name, src->table_name, sizeof(dst->table_name));
    memcpy(dst->column_name, src->column_name, sizeof(dst->column_name));
    memcpy(dst->alias_fqdn, src->alias_fqdn, sizeof(dst->alias_fqdn));
    dst->is_encrypted = src->is_encrypted;
    dst->column_oid = src->column_oid;
    dst->table_oid = src->table_oid;
    dst->location = src->location;
    memcpy(dst->dek_name, src->dek_name, sizeof(dst->dek_name));
    memcpy(dst->support_algo, src->support_algo, sizeof(dst->support_algo));
    /* ReqID:TECC4.0 - pending_tables support */
    memcpy(dst->original_type_name, src->original_type_name, sizeof(dst->original_type_name));

    return dst;
}

/*
 * Compare two column reference data objects
 */
bool
tecc_column_ref_data_equal(const TeccColumnRefData *data1,
                          const TeccColumnRefData *data2)
{
    if (data1 == NULL || data2 == NULL)
        return false;

    /* Compare name components */
    if (strcmp(data1->catalog_name, data2->catalog_name) != 0)
        return false;
    if (strcmp(data1->schema_name, data2->schema_name) != 0)
        return false;
    if (strcmp(data1->table_name, data2->table_name) != 0)
        return false;
    if (strcmp(data1->column_name, data2->column_name) != 0)
        return false;

    /* Compare other fields */
    if (data1->is_encrypted != data2->is_encrypted)
        return false;
    if (data1->column_oid != data2->column_oid)
        return false;
    if (data1->table_oid != data2->table_oid)
        return false;

    return true;
}

/*
 * Convert to string representation
 */
char *
tecc_column_ref_data_to_string(const TeccColumnRefData *data)
{
    size_t len;
    char *str;

    if (data == NULL)
        return pg_strdup("(null)");

    /* Calculate required buffer size */
    len = strlen(data->alias_fqdn) + 64; /* Extra space for metadata */
    str = pg_malloc(len);
    if (str == NULL)
        return NULL;

    /* Format string */
    snprintf(str, len, "%s [encrypted=%s, col_oid=%u, tab_oid=%u, loc=%d]",
             data->alias_fqdn,
             data->is_encrypted ? "true" : "false",
             data->column_oid,
             data->table_oid,
             data->location);

    return str;
}

/*
 * Internal helper function: Construct fully qualified domain name
 */
static void
tecc_concat_col_fqdn(TeccColumnRefData *data)
{
    char *ptr;
    size_t remaining;

    if (data == NULL)
        return;

    ptr = data->alias_fqdn;
    remaining = NAMEDATALEN * 4;  /* alias_fqdn size */

    /* Start with empty string */
    data->alias_fqdn[0] = '\0';

    /* Catalog name */
    if (data->catalog_name[0] != '\0')
    {
        strncpy(ptr, data->catalog_name, remaining - 1);
        ptr[remaining - 1] = '\0';
        ptr += strlen(data->catalog_name);
        remaining -= strlen(data->catalog_name);

        if (remaining > 1)
        {
            *ptr++ = '.';
            remaining--;
        }
    }

    /* Schema name */
    if (data->schema_name[0] != '\0')
    {
        strncpy(ptr, data->schema_name, remaining - 1);
        ptr[remaining - 1] = '\0';
        ptr += strlen(data->schema_name);
        remaining -= strlen(data->schema_name);

        if (remaining > 1)
        {
            *ptr++ = '.';
            remaining--;
        }
    }

    /* Table name */
    if (data->table_name[0] != '\0')
    {
        strncpy(ptr, data->table_name, remaining - 1);
        ptr[remaining - 1] = '\0';
        ptr += strlen(data->table_name);
        remaining -= strlen(data->table_name);

        if (remaining > 1)
        {
            *ptr++ = '.';
            remaining--;
        }
    }

    /* Column name */
    if (data->column_name[0] != '\0')
    {
        strncpy(ptr, data->column_name, remaining - 1);
        ptr[remaining - 1] = '\0';
    }
}
/* End - ReqID:TECC4.0 */