/*-------------------------------------------------------------------------
 *
 * mys_keywords.h
 *	  HighGo's list of SQL keywords
 *
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/include/mysql_parser/mys_keywords.h
 *
 * add the file for requirement "SRS-MISC-MYSQL-FRAME"
 *
 *-------------------------------------------------------------------------
 */
#ifndef MYS_KEYWORDS_H
#define MYS_KEYWORDS_H

#include "common/kwlookup.h"

/* Keyword categories --- should match lists in gram.y */
#define UNRESERVED_KEYWORD		0
#define COL_NAME_KEYWORD		1
#define TYPE_FUNC_NAME_KEYWORD	2
#define RESERVED_KEYWORD		3

#ifndef FRONTEND
extern const ScanKeywordList MysScanKeywords;
extern const uint8 MysScanKeywordCategories[];
extern const bool MysScanKeywordBareLabel[];
#else
extern const ScanKeywordList MysScanKeywords;
extern const uint8 MysScanKeywordCategories[];
extern const bool MysScanKeywordBareLabel[];
#endif

#endif							/* MYS_KEYWORDS_H */
