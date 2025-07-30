#include "postgres.h"

#include "fmgr.h"
#include "funcapi.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/numeric.h"

PG_FUNCTION_INFO_V1(interval);

Datum 
interval(PG_FUNCTION_ARGS)
{
    int	        nargs;
	Datum	   *args;
	Oid        *types;
	bool       *nulls;
    int         ret = 0;

    nargs = extract_variadic_args(fcinfo, 0, false, &args, &types, &nulls);

    if (nargs < 2)
    {
        ereport(ERROR,
            (errcode(ERRCODE_SYNTAX_ERROR),
             errmsg("You have an error in your SQL syntax")));
    }

    if (nulls[0])
		PG_RETURN_INT32(-1);

    for (int i = 1; i < nargs; i++)
    {
        if (!nulls[i])
        {
            bool res = DatumGetBool(DirectFunctionCall2(numeric_gt,
                                                        args[i],
                                                        args[0]));

            if (res)
                break;
        }

        ret++;
    }

    pfree(args);
    pfree(types);
    pfree(nulls);

    PG_RETURN_INT32(ret);
}
