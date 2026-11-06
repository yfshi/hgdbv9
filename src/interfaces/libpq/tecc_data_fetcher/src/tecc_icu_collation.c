/*-------------------------------------------------------------------------
 *
 * tecc_icu_collation.c
 *    ICU collation support for TECC
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/interfaces/libpq/tecc_data_fetcher/src/tecc_icu_collation.c
 *
 * ReqID:TECC4.0 - ICU collation support
 * Authored by <wanghaoyan@highgo.com>, 20260408.
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *-------------------------------------------------------------------------
 */

#include "postgres_fe.h"
#include "c.h"
#include "tecc_icu_collation.h"

/* Stub implementation - actual ICU collation not implemented yet */
int
tecc_icu_collation_init(void)
{
    return 0;
}

void
tecc_icu_collation_cleanup(void)
{
}

int
tecc_icu_collation_compare(const char *str1, const char *str2,
                          const char *collation_name)
{
    /* Simple fallback: use strcmp */
    return strcmp(str1, str2);
}

bool
tecc_icu_collation_is_supported(const char *collation_name)
{
    return false;
}