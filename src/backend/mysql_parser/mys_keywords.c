/*-------------------------------------------------------------------------
 *
 * mys_keywords.c
 *	  HighGo's list of SQL keywords (mysql Compatible)
 *
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
 *
 *
 * IDENTIFICATION
 *	  src/backend/mysql_parser/mys_keywords.c
 *
 * add the file for requirement "SRS-MISC-MYSQL-FRAME"
 *
 *-------------------------------------------------------------------------
 */
#include "c.h"

#include "mysql_parser/mys_keywords.h"


/* ScanKeywordList lookup data for SQL keywords */

#include "mys_kwlist_d.h"

/* Keyword categories for SQL keywords */

#define PG_KEYWORD(kwname, value, category, collabel) category,

const uint8 MysScanKeywordCategories[MYSSCANKEYWORDS_NUM_KEYWORDS] = {
#include "mysql_parser/mys_kwlist.h"
};

#undef PG_KEYWORD

/* Keyword can-be-bare-label flags for SQL keywords */

#define PG_KEYWORD(kwname, value, category, collabel) collabel,

#define BARE_LABEL true
#define AS_LABEL false

const bool	MysScanKeywordBareLabel[MYSSCANKEYWORDS_NUM_KEYWORDS] = {
#include "mysql_parser/mys_kwlist.h"
};

#undef PG_KEYWORD
#undef BARE_LABEL
#undef AS_LABEL
