/*--------------------------------------------------------------------
 * 
 * File: ivorysql_my.h
 *
 * Abstract: 
 * 		This file contains extern declarations for ivorysql_my itself.
 *
 * Authored by qinshiyu@highgo.com, 20230803.
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_my/src/include/ivorysql_my.h
 *
 * add the file for requirement "SRS-MYSQL-CASE"
 *
 *--------------------------------------------------------------------
 */
#ifndef IVORYSQL_MYS_H_
#define IVORYSQL_MYS_H_

#include "postgres.h"
#include "fmgr.h" /* ReqID:SRS-MYSQL-BUILDINFUNCTION */
#include "utils/varbit.h"

#define SIGNED_FLAG                     0
#define UNSIGNED_FLAG                   1

#define VAL(CH)			                ((CH) - '0')

#define YEAR4_LEN                       4
#define YEAR2_LEN                       2
#define YEAR2_BOUND_BETWEEN_20C_21C     70
#define YEAR2_IN_RANGE(x)               ((x) >= 0 && (x) <= 99)

#define A_LESS_THAN_B                   (-1)
#define A_GREATER_THAN_B                (1)
#define A_EQUAL_B                       (0)

typedef unsigned char uchar;

extern Datum hexstring_varbinary(const char *value);/* BUG#5048 5070 */
extern char *my_bit_out(VarBit *s, int32 atttypmod);
extern int16 my_atoi_int2(const char *s);
extern int32 my_atoi_int4(const char *s);
extern uint64 my_strntoull10rnd_8bit(const char *str, size_t length, int unsigned_flag, char **endptr, int *error);
extern uint64 pg_atoi_uint(const char *s,int c, int sizenum, bool seterr);
/* Begin - ReqID:SRS-MYSQL-BUILDINFUNCTION */
extern Datum pgtime_mystimes(PG_FUNCTION_ARGS);
extern Datum pgtimestamp_mystimestamps(PG_FUNCTION_ARGS);
/* End - ReqID:SRS-MYSQL-BUILDINFUNCTION */
/* Begin - ReqID:SRS-MYSQL-BUILDINFUNCTION */
extern bool extract_leading_integer_str(const char *str, char *out_buf, size_t size);
extern Datum mysuint2_in(PG_FUNCTION_ARGS);
extern Datum mysuint4_in(PG_FUNCTION_ARGS);
extern Datum mysuint8_in(PG_FUNCTION_ARGS);
extern Datum mysdate_in(PG_FUNCTION_ARGS);
extern Datum mysdate_out(PG_FUNCTION_ARGS);
extern Datum mystime_in(PG_FUNCTION_ARGS);
extern Datum mystime_out(PG_FUNCTION_ARGS);
extern Datum mysdatetime_in(PG_FUNCTION_ARGS);
extern Datum mysdatetime_out(PG_FUNCTION_ARGS);
extern Datum mystimestamp_out(PG_FUNCTION_ARGS);
/* End - ReqID:SRS-MYSQL-BUILDINFUNCTION */
#endif	/* IVORYSQL_MYS_H_ */
