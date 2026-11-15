/* Begin - ReqID:TECC4.0 */
/*
 * File: column_ref_data.h
 *
 * Abstract:
 *   Column reference data management for TECC expression processor module.
 *   Provides functions for creating, initializing, and managing TeccColumnRefData
 *   structures used in SQL expression processing.
 *
 * Authored by <wangxp@highgo.com>, 20260404.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_expression/column_ref_data.h
 */

#ifndef TECC_COLUMN_REF_DATA_H
#define TECC_COLUMN_REF_DATA_H

#include <stdbool.h>
#include <stddef.h>
#include "../frontend_parser/fe_adapter.h"  /* For TeccColumnRefData definition */

/* TeccColumnRefData is defined in fe_adapter.h */

/*
 * Column reference data creation and management functions
 */

/* Create a new TeccColumnRefData object */
TeccColumnRefData *tecc_column_ref_data_create(void);

/* Destroy a TeccColumnRefData object */
void tecc_column_ref_data_destroy(TeccColumnRefData *data);

/* Initialize a TeccColumnRefData object with names */
void tecc_column_ref_data_init(TeccColumnRefData *data,
                              const char *catalog,
                              const char *schema,
                              const char *table,
                              const char *column);

/* ReqID:TECC4.0 - Set table alias (for multi-table UPDATE/DELETE FROM/USING) */
void tecc_column_ref_data_set_alias(TeccColumnRefData *data, const char *alias_name);

/* Bug 9323 - Set the name under which a subquery/CTE exposes this column
 * (SELECT col AS n / WITH cte(n) AS ... / FROM (...) s(n)). */
void tecc_column_ref_data_set_column_alias(TeccColumnRefData *data, const char *column_alias);

/* Bug 9323 - Name the parent query can use for this column:
 * column_alias when renamed, column_name otherwise. */
const char *tecc_column_ref_data_get_exposed_name(const TeccColumnRefData *data);

/* Check if column reference is fully qualified */
bool tecc_column_ref_data_is_fqdn(const TeccColumnRefData *data);

/* Get the fully qualified domain name */
const char *tecc_column_ref_data_get_fqdn(const TeccColumnRefData *data);

/* Check if column is encrypted */
bool tecc_column_ref_data_is_encrypted(const TeccColumnRefData *data);

/* Set column encryption status */
void tecc_column_ref_data_set_encrypted(TeccColumnRefData *data, bool encrypted);

/* Get column OID */
Oid tecc_column_ref_data_get_column_oid(const TeccColumnRefData *data);

/* Set column OID */
void tecc_column_ref_data_set_column_oid(TeccColumnRefData *data, Oid column_oid);

/* Get table OID */
Oid tecc_column_ref_data_get_table_oid(const TeccColumnRefData *data);

/* Set table OID */
void tecc_column_ref_data_set_table_oid(TeccColumnRefData *data, Oid table_oid);

/* Get location in SQL */
int tecc_column_ref_data_get_location(const TeccColumnRefData *data);

/* Set location in SQL */
void tecc_column_ref_data_set_location(TeccColumnRefData *data, int location);

/* Get DEK name */
const char *tecc_column_ref_data_get_dek_name(const TeccColumnRefData *data);

/* Set DEK name */
void tecc_column_ref_data_set_dek_name(TeccColumnRefData *data, const char *dek_name);

/* Get supported algorithms */
const char *tecc_column_ref_data_get_support_algo(const TeccColumnRefData *data);

/* Set supported algorithms */
void tecc_column_ref_data_set_support_algo(TeccColumnRefData *data, const char *support_algo);

/* ReqID:TECC4.0 - 一列多 DEK：he-add / he-mul DEK 存取 */
const char *tecc_column_ref_data_get_he_add_dek_name(const TeccColumnRefData *data);
void tecc_column_ref_data_set_he_add_dek_name(TeccColumnRefData *data, const char *dek_name);
const char *tecc_column_ref_data_get_he_mul_dek_name(const TeccColumnRefData *data);
void tecc_column_ref_data_set_he_mul_dek_name(TeccColumnRefData *data, const char *dek_name);

/* Copy column reference data */
TeccColumnRefData *tecc_column_ref_data_copy(const TeccColumnRefData *src);

/* Compare two column reference data objects */
bool tecc_column_ref_data_equal(const TeccColumnRefData *data1,
                               const TeccColumnRefData *data2);

/* Convert to string representation */
char *tecc_column_ref_data_to_string(const TeccColumnRefData *data);

#endif /* TECC_COLUMN_REF_DATA_H */
/* End - ReqID:TECC4.0 */