/* Begin - ReqID:TECC4.0 */
/*-------------------------------------------------------------------------
 *
 * tecc_icu_collation.h
 *    ICU collation support for TECC
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/interfaces/libpq/tecc_data_fetcher/include/tecc_icu_collation.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_ICU_COLLATION_H
#define TECC_ICU_COLLATION_H

#include "tecc_data_fetcher.h"

/* ICU collation functions */

/* Initialize ICU collation subsystem */
extern int tecc_icu_collation_init(void);

/* Cleanup ICU collation subsystem */
extern void tecc_icu_collation_cleanup(void);

/* Compare two strings using ICU collation */
extern int tecc_icu_collation_compare(const char *str1, const char *str2,
                                     const char *collation_name);

/* Check if a collation is supported by ICU */
extern bool tecc_icu_collation_is_supported(const char *collation_name);

#endif /* TECC_ICU_COLLATION_H */
/* End - ReqID:TECC4.0 */