#ifndef __MYYEAR_H__
#define __MYYEAR_H__
#include <stdint.h>

typedef uint8_t YearADT;

#define DatumGetYearADT(x)      ((YearADT)DatumGetInt16(x))
#define PG_GETARG_YEARADT(n)    DatumGetYearADT(PG_GETARG_DATUM(n))

#define MAX_YEAR_NUM    2155
#define MIN_YEAR_NUM    1901
#define YEAR4_LEN       4
#define YEAR2_LEN       2
#define MAX_YEAR2_NUM   99
#define MIN_YEAR2_NUM   0

#define YEAR2_BOUND_BETWEEN_20C_21C 70

#define YEAR4_IN_RANGE(x)   ((x) >= MIN_YEAR_NUM && (x) <= MAX_YEAR_NUM)
#define YEAR2_IN_RANGE(x)   ((x) >= 0 && (x) <= 99)
#define YEAR_VALID(x)       (YEAR4_IN_RANGE(x) || YEAR2_IN_RANGE(x))

/* YEAR(2) -> YEAR(4) or YEAR(4) -> YEAR(2) */
#define YEAR_CONVERT(x)     ((x) = (-x))
#define IS_YEAR2(x)         ((x) < 0)
#define IS_YEAR4(x)         ((x) >= 0 )

#define YearADTGetDatum(X)      Int16GetDatum(X)
#define PG_RETURN_YEARADT(x)    return YearADTGetDatum(x)

#define Year_to_YearADT(x)  ((x) - MIN_YEAR_NUM + 1)
#define YearADT_to_Year(x)  ((x) == 0 ? 0 : (int32)(x) + MIN_YEAR_NUM - 1)

#define TYPMODOUT_LEN   64

/* Datetime limits */
/* lower bound: 1753-01-01 00:00:00.000 */
#define MIN_DATETIME	INT64CONST(-7794489600000000)
/* upper bond: 9999-12-31 23:59:29.999 */
#define END_DATETIME	INT64CONST(252455615999999000)
#define IS_VALID_DATETIME(t)  (MIN_DATETIME <= (t) && (t) < END_DATETIME)


#endif // end __MYYEAR_H__