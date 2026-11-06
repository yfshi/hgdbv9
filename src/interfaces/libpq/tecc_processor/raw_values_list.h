/* Begin - ReqID:TECC4.0 */
/*
 * File: raw_values_list.h
 *
 * Abstract:
 *   Raw values list management for TECC processor module.
 *   Provides list operations for managing collections of raw values.
 *
 * Authored by <wangxp@highgo.com>, 20260403.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/raw_values_list.h
 */

#ifndef RAW_VALUES_LIST_H
#define RAW_VALUES_LIST_H

#include "tecc_processor_types.h"  /* Include type definitions */

/*
 * Raw values list interface functions
 */

/* Create new raw values list */
TeccRawValuesList *tecc_raw_values_list_create(int initial_capacity);

/* Destroy raw values list */
void tecc_raw_values_list_destroy(TeccRawValuesList *list);

/* Add value to list */
int tecc_raw_values_list_add(TeccRawValuesList *list, TeccRawValue *value);

/* Get value from list by index */
TeccRawValue *tecc_raw_values_list_get(const TeccRawValuesList *list, int index);

/* Remove value from list by index */
int tecc_raw_values_list_remove(TeccRawValuesList *list, int index);

/* Get number of values in list */
int tecc_raw_values_list_count(const TeccRawValuesList *list);

/* Check if list is empty */
bool tecc_raw_values_list_is_empty(const TeccRawValuesList *list);

/* Clear all values from list */
void tecc_raw_values_list_clear(TeccRawValuesList *list);

/* Find value by column name */
TeccRawValue *tecc_raw_values_list_find_by_column(const TeccRawValuesList *list,
                                                  const char *column_name);

/* Process all values in list */
int tecc_raw_values_list_process_all(TeccRawValuesList *list);

/* Get list of column names */
List *tecc_raw_values_list_get_column_names(const TeccRawValuesList *list);

#endif /* RAW_VALUES_LIST_H */
/* End - ReqID:TECC4.0 */