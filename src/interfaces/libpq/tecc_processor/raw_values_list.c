#include "postgres_fe.h"
#include "common/fe_memutils.h"
/* Begin - ReqID:TECC4.0 */
/*
 * File: raw_values_list.c
 *
 * Abstract:
 *   Raw values list implementation for TECC processor module.
 *   Provides list operations for managing collections of raw values.
 *
 * Authored by <wangxp@highgo.com>, 20260403.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/raw_values_list.c
 */

#include "raw_values_list.h"
#include "raw_value.h"  /* For tecc_raw_value_destroy and tecc_raw_value_process */
#include "internal/tecc_processor_internal.h"  /* For debug functions and utilities */
#include "frontend_parser/tecc_nodes.h"  /* For frontend-compatible list operations */
#include <string.h>  /* For memcpy, strcmp */

#define DEFAULT_CAPACITY 16

/*
 * Create new raw values list
 */
TeccRawValuesList *
tecc_raw_values_list_create(int initial_capacity)
{
    TeccRawValuesList *list;

    if (initial_capacity <= 0)
        initial_capacity = DEFAULT_CAPACITY;

    list = pg_malloc0(sizeof(TeccRawValuesList));
    if (list == NULL)
        return NULL;

    list->values = pg_malloc0(initial_capacity * sizeof(TeccRawValue *));
    if (list->values == NULL)
    {
        pg_free(list);
        return NULL;
    }

    list->num_values = 0;
    list->max_values = initial_capacity;
    list->total_size = 0;
    list->values_list = NIL;  /* Initialize PostgreSQL List */
    list->last_error = 0;

    return list;
}

/*
 * Destroy raw values list
 */
void
tecc_raw_values_list_destroy(TeccRawValuesList *list)
{
    int i;

    if (list == NULL)
        return;

    /* Destroy all values in the list */
    if (list->values != NULL)
    {
        for (i = 0; i < list->num_values; i++)
        {
            if (list->values[i] != NULL)
                tecc_raw_value_destroy(list->values[i]);
        }
        pg_free(list->values);
    }

    /* Free frontend-compatible list if exists */
    if (list->values_list != NIL)
    {
        list_free(list->values_list);
    }

    pg_free(list);
}

/*
 * Add value to list
 */
int
tecc_raw_values_list_add(TeccRawValuesList *list, TeccRawValue *value)
{
    TeccRawValue **new_values;
    int new_capacity;

    if (list == NULL || value == NULL)
        return -1;

    /* Check if we need to expand the array */
    if (list->num_values >= list->max_values)
    {
        new_capacity = list->max_values * 2;
        new_values = pg_malloc0(new_capacity * sizeof(TeccRawValue *));
        if (new_values == NULL)
            return -1;

        /* Copy existing values */
        if (list->values != NULL)
        {
            memcpy(new_values, list->values, list->num_values * sizeof(TeccRawValue *));
            pg_free(list->values);
        }

        list->values = new_values;
        list->max_values = new_capacity;
    }

    /* Add the new value */
    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "raw_values_list_add: before add: list=%p, num_values=%d, max_values=%d, value=%p, type=%d",
                        list, list->num_values, list->max_values, value, value ? value->type : -1);
    list->values[list->num_values] = value;
    list->total_size += value->length;
    list->num_values++;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "raw_values_list_add: after add: num_values=%d, returning index=%d",
                        list->num_values, list->num_values - 1);

    /* Also add to PostgreSQL List for compatibility */
    list->values_list = lappend(list->values_list, value);

    return list->num_values - 1;
}

/*
 * Get value from list by index
 */
TeccRawValue *
tecc_raw_values_list_get(const TeccRawValuesList *list, int index)
{
    if (list == NULL || index < 0 || index >= list->num_values)
        return NULL;

    return list->values[index];
}

/*
 * Remove value from list by index
 */
int
tecc_raw_values_list_remove(TeccRawValuesList *list, int index)
{
    int i;

    if (list == NULL || index < 0 || index >= list->num_values)
        return -1;

    /* Destroy the value at the index */
    if (list->values[index] != NULL)
    {
        list->total_size -= list->values[index]->length;
        tecc_raw_value_destroy(list->values[index]);
    }

    /* Shift remaining values */
    for (i = index; i < list->num_values - 1; i++)
    {
        list->values[i] = list->values[i + 1];
    }

    list->num_values--;
    list->values[list->num_values] = NULL;

    /* Remove from frontend-compatible list */
    if (list->values_list != NIL)
    {
        list->values_list = list_delete_nth_cell(list->values_list, index);
    }

    return 0;
}

/*
 * Get number of values in list
 */
int
tecc_raw_values_list_count(const TeccRawValuesList *list)
{
    if (list == NULL)
        return 0;

    return list->num_values;
}

/*
 * Check if list is empty
 */
bool
tecc_raw_values_list_is_empty(const TeccRawValuesList *list)
{
    if (list == NULL)
        return true;

    return (list->num_values == 0);
}

/*
 * Clear all values from list
 */
void
tecc_raw_values_list_clear(TeccRawValuesList *list)
{
    int i;

    if (list == NULL)
        return;

    /* Destroy all values */
    if (list->values != NULL)
    {
        for (i = 0; i < list->num_values; i++)
        {
            if (list->values[i] != NULL)
            {
                tecc_raw_value_destroy(list->values[i]);
                list->values[i] = NULL;
            }
        }
    }

    list->num_values = 0;
    list->total_size = 0;
    list->values_list = NIL;  /* Reset PostgreSQL List */
}

/*
 * Find value by column name
 */
TeccRawValue *
tecc_raw_values_list_find_by_column(const TeccRawValuesList *list,
                                                  const char *column_name)
{
    int i;
    TeccRawValue *value;

    if (list == NULL || column_name == NULL)
        return NULL;

    for (i = 0; i < list->num_values; i++)
    {
        value = list->values[i];
        if (value != NULL && value->column_name != NULL)
        {
            if (strcmp(value->column_name, column_name) == 0)
                return value;
        }
    }

    return NULL;
}

/*
 * Process all values in list
 */
int
tecc_raw_values_list_process_all(TeccRawValuesList *list)
{
    int i;
    int result;
    TeccRawValue *value;

    if (list == NULL)
        return -1;

    for (i = 0; i < list->num_values; i++)
    {
        value = list->values[i];
        if (value != NULL)
        {
            result = tecc_raw_value_process(value);
            if (result != 0)
            {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Failed to process value at index %d", i);
                return result;
            }
        }
    }

    return 0;
}

/*
 * Get list of column names
 */
List *
tecc_raw_values_list_get_column_names(const TeccRawValuesList *list)
{
    List *column_names;
    int i;
    TeccRawValue *value;

    if (list == NULL)
        return NIL;

    column_names = NIL;

    for (i = 0; i < list->num_values; i++)
    {
        value = list->values[i];
        if (value != NULL && value->column_name != NULL)
        {
            column_names = tecc_processor_list_append_string(column_names,
                                                            value->column_name);
        }
    }

    return column_names;
}
/* End - ReqID:TECC4.0 */