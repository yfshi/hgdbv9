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
#include "utils/date.h"
#include "utils/guc.h"
#include "utils/numeric.h"
#include "utils/varbit.h"

#define VAL(CH)			                ((CH) - '0')

#define YEAR4_LEN                       4
#define YEAR2_LEN                       2
#define YY_PART_YEAR     				70
#define YEAR2_IN_RANGE(x)               ((x) >= 0 && (x) <= 99)

#define YearADTGetDatum(X)      		Int16GetDatum(X)
#define PG_RETURN_YEARADT(x)    		return YearADTGetDatum(x)

#define A_LESS_THAN_B                   (-1)
#define A_GREATER_THAN_B                (1)
#define A_EQUAL_B                       (0)

#define MYSQL_CHECK_DIVISION_BY_ZERO(val)                                                       \
	{                                                                                           \
		if (val == 0)                                                                           \
		{                                                                                       \
			if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))                                  \
				ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0"))); \
			PG_RETURN_NULL();                                                                   \
		}                                                                                       \
	}

typedef unsigned char uchar;

extern int8   my_atoi_int8(const char *s,  bool is_insert);
extern int16  my_atoi_int16(const char *s, bool is_insert);
extern int32  my_atoi_int24(const char *s, bool is_insert);
extern int32  my_atoi_int32(const char *s, bool is_insert);
extern int64  my_atoi_int64(const char *s, bool is_insert);
extern uint8  my_atoi_uint8(const char *s, bool is_insert);
extern uint16 my_atoi_uint16(const char *s, bool is_insert);
extern uint32 my_atoi_uint24(const char *s, bool is_insert);
extern uint32 my_atoi_uint32(const char *s, bool is_insert);
extern uint64 my_atoi_uint64(const char *s, bool is_insert);
extern float  my_atof(const char *src, bool is_insert);
extern double my_atod(const char *src, bool is_insert);
extern Numeric my_aton(const char *src, int typmod, bool is_insert);

extern Datum parse_hexstring(const char *value);/* BUG#5048 5070 */
extern Datum parse_bitstring(const char *value); /* SRS-MYSQL-HEX-LITERAL */
extern char *my_bit_out(VarBit *s, int32 atttypmod);
/* Begin - ReqID:SRS-MYSQL-BUILDINFUNCTION */
extern Datum pgtime_mystimes(PG_FUNCTION_ARGS);
extern Datum pgtimestamp_mystimestamps(PG_FUNCTION_ARGS);
extern bool extract_leading_integer_str(const char *str, char *out_buf, size_t size);
extern Datum uint2_in(PG_FUNCTION_ARGS);
extern Datum uint4_in(PG_FUNCTION_ARGS);
extern Datum uint8_in(PG_FUNCTION_ARGS);
extern Datum year_out(PG_FUNCTION_ARGS);
extern Datum mysdate_in(PG_FUNCTION_ARGS);
extern Datum mysdate_out(PG_FUNCTION_ARGS);
extern Datum mystime_in(PG_FUNCTION_ARGS);
extern Datum mystimes_in(PG_FUNCTION_ARGS);
extern Datum mystime_out(PG_FUNCTION_ARGS);
extern Datum mysdatetime_in(PG_FUNCTION_ARGS);
extern Datum mysdatetime_out(PG_FUNCTION_ARGS);
extern Datum mystimestamp_out(PG_FUNCTION_ARGS);
extern Datum mysdatetimes_in(PG_FUNCTION_ARGS);
extern Datum mysdatetimes_out(PG_FUNCTION_ARGS);
extern Datum pgtimestamp_mysdatetimes(PG_FUNCTION_ARGS);
/* End - ReqID:SRS-MYSQL-BUILDINFUNCTION */

/* Begin - bug#6619 */
extern Datum bittobinary(PG_FUNCTION_ARGS);
/* End - bug#6619 */

extern YearADT bigint_to_YearADT(int64 year);
extern Oid mysql_agg_field_type(List *exprs, Node **which_expr); /* Bug - #6172 */
#endif	/* IVORYSQL_MYS_H_ */
