/*-------------------------------------------------------------------------
 *
 * NUMBER-processing utility routines for frontend code (Oracle compatibility)
 *
 *
 * Portions Copyright (c) 2023, IvorySQL-2.0
 *
 * src/include/oracle_fe_utils/ora_number_utils.h
 *
 * add the file for compatible requirement "TO_NUMBER()"
 *
 *-------------------------------------------------------------------------
 */
#ifndef ORA_NUMBER_UTILS_H
#define ORA_NUMBER_UTILS_H

#include "libpq-fe.h"
#include "utils/ora_compatible.h"

/* Functions */
extern bool ora_numwidth_format(PGconn *conn, PGresult **results);

#endif							/* ORA_NUMBER_UTILS_H */
