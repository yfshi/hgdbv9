/* Begin ReqID: SRS-CLIENT-AUTH */

/*-------------------------------------------------------------------------
 *
 * restrict.h
 *	  restrict some use, like MAC address and time.
 *
 *
 * Portions Copyright (c) 1996-2019, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * Portions Copyright (c) 2009-2023, HighGo Software Co.,Ltd. All rights reserved.
 * src/include/utils/restrict.h
 *
 *-------------------------------------------------------------------------
 */

#ifndef RESTRICT_H
#define RESTRICT_H

typedef void (*check_connrules_hook_type) (char* username, char* mac_address);
extern PGDLLIMPORT check_connrules_hook_type check_rules_hook;
extern bool enable_macandtime;

#endif							/* RESTRICT_H */
/* End ReqID: SRS-CLIENT-AUTH */
