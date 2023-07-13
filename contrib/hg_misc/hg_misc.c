/*------------------------------------------------------------------
 *
 * File: hg_misc.c
 *
 * Abstract:
 * 	  Display the version of the Highgo database.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *		contrib/hg_misc/hg_misc.c
 *
 * Requirement description:
 *		ReqID:SRS-MISC-LICENSE
 
 *------------------------------------------------------------------
 */

#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"

#ifdef USE_LICENSE
#include "license.h"
#endif

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

PG_FUNCTION_INFO_V1(highgo_version);

Datum highgo_version(PG_FUNCTION_ARGS)
{
#ifdef USE_LICENSE
	hg_set_is_with_title(false);
	PG_RETURN_TEXT_P(cstring_to_text(hg_get_highgoVersion()));
#endif
	PG_RETURN_TEXT_P(cstring_to_text("without license is not show version of highgo"));
}
