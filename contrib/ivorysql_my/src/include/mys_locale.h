/*-------------------------------------------------------------------------------
 * 
 * File: mys_locale.h
 *
 * Abstract: 
 * 		lc_locale_time related settings.
 *
 * Authored by jinhuajian@highgo.com, 20250311.
 *
 * Copyright:
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_my/src/include/mys_locale.h
 *
 *-------------------------------------------------------------------------------
 */
#ifndef MYSLOCALE_H
#define MYSLOCALE_H
		
#define LOCALE_LEN 6	/* length of locale name with '\0' ended. e.g. "en_US" */
		
#include "postgres.h"
#include "utils/dynahash.h"
#include "utils/hsearch.h"

typedef struct MyLocale
{
	char	locale_name[LOCALE_LEN];
	char	decimal_point;
	char	thousand_sep;
	char  **month_names;
	char  **ab_month_names;
	char  **day_names;
	char  **ab_day_names;
}MyLocale;

MyLocale *MyLocaleSearch(char* target);

int MyMonthSearch(char *target, char **input);

#endif  // MYSLOCALE_H

