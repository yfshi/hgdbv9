/*-------------------------------------------------------------------------
 * 
 * File: datetime_func.h
 *
 * Abstract: 
 * 		This file contains extern declarations for datetime functions.
 *
 * Authored by linjh@highgo.com, 20220510.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_my/src/include/datetime_func.h
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "c.h"
#include "datatype/timestamp.h"
#include "pgtime.h"
#include "utils/date.h"
#include "utils/datetime.h"
#include "utils/guc.h"
#include "utils/numeric.h"
#include "../datatype/myyear.h"



extern bool str_datetime_in_with_flag_internal(char *str, struct pg_tm *result_tm, fsec_t *result_fsec, mysql_dttype *result_tm_type);
extern bool num_datetime_in_with_flag_internal(Numeric num, struct pg_tm *result_tm, fsec_t *result_fsec, mysql_dttype *result_tm_type);
extern bool isValidDatetimeFormat(const char *datetime);