#include "postgres.h"

#include "fmgr.h"
#include "utils/fmgrprotos.h"
#include "utils/guc.h"
#include "utils/numeric.h"

PG_FUNCTION_INFO_V1(numeric_eq_int4);
PG_FUNCTION_INFO_V1(numeric_eq_int8);
PG_FUNCTION_INFO_V1(numeric_add_int4);
PG_FUNCTION_INFO_V1(int4_add_numeric);
PG_FUNCTION_INFO_V1(numeric_add_int8);
PG_FUNCTION_INFO_V1(int8_add_numeric);
/* Begin - bug#z6039 */
PG_FUNCTION_INFO_V1(uint8_mul_numeric);
PG_FUNCTION_INFO_V1(numeric_mul_uint8);
/* End - bug#z6039 */

Datum
numeric_eq_int4(PG_FUNCTION_ARGS)
{
    Datum val1 = PG_GETARG_DATUM(0);
    int32 val2 = PG_GETARG_INT32(1);

    return DirectFunctionCall2(numeric_eq, val1, NumericGetDatum(int64_to_numeric((int64) val2)));
}

Datum
numeric_eq_int8(PG_FUNCTION_ARGS)
{
    Datum val1 = PG_GETARG_DATUM(0);
    int64 val2 = PG_GETARG_INT64(1);

    return DirectFunctionCall2(numeric_eq, val1, NumericGetDatum(int64_to_numeric(val2)));
}

Datum
numeric_add_int4(PG_FUNCTION_ARGS)
{
    Datum val1 = PG_GETARG_DATUM(0);
    int32 val2 = PG_GETARG_INT32(1);

    return DirectFunctionCall2(numeric_add, val1, NumericGetDatum(int64_to_numeric((int64) val2)));
}

Datum
numeric_add_int8(PG_FUNCTION_ARGS)
{
    Datum val1 = PG_GETARG_DATUM(0);
    int64 val2 = PG_GETARG_INT64(1);

    return DirectFunctionCall2(numeric_add, val1, NumericGetDatum(int64_to_numeric(val2)));
}

Datum
int4_add_numeric(PG_FUNCTION_ARGS)
{
    int64 val1 = PG_GETARG_INT32(0);
    Datum val2 = PG_GETARG_DATUM(1);

    return DirectFunctionCall2(numeric_add, NumericGetDatum(int64_to_numeric(val1)), val2);
}

Datum
int8_add_numeric(PG_FUNCTION_ARGS)
{
    int64 val1 = PG_GETARG_INT64(0);
    Datum val2 = PG_GETARG_DATUM(1);

    return DirectFunctionCall2(numeric_add, NumericGetDatum(int64_to_numeric(val1)), val2);
}

/* Begin - bug#z6039 */
Datum
uint8_mul_numeric(PG_FUNCTION_ARGS)
{
    Datum arg1 = PG_GETARG_DATUM(0);    // uint64
    Datum arg2 = PG_GETARG_DATUM(1);    // numeric

    Datum tmp = NumericGetDatum(uint64_to_numeric(DatumGetUInt64(arg1)));
    Datum ret = DirectFunctionCall2(numeric_mul, tmp, arg2);

    PG_RETURN_DATUM(ret);
}

Datum
numeric_mul_uint8(PG_FUNCTION_ARGS)
{
    Datum arg1 = PG_GETARG_DATUM(0);    // numeric
    Datum arg2 = PG_GETARG_DATUM(1);    // uint64

    Datum tmp = NumericGetDatum(uint64_to_numeric(DatumGetUInt64(arg2)));
    Datum ret = DirectFunctionCall2(numeric_mul, tmp, arg1);

    PG_RETURN_DATUM(ret);
}
/* End - bug#z6039 */