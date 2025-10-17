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