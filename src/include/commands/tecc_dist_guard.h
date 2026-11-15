/*-------------------------------------------------------------------------
 *
 * tecc_dist_guard.h
 *	  Reject encrypted columns as the distribution column (peripheral W6 guard).
 *
 * ReqID:TECC4.0
 *
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd.
 * Author: <wangxp@highgo.com>
 *
 * IDENTIFICATION
 *	  src/include/commands/tecc_dist_guard.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_DIST_GUARD_H
#define TECC_DIST_GUARD_H

/* ReqID:TECC4.0 - install the distribution-column guard hook */
extern void tecc_install_dist_guard_hooks(void);

#endif							/* TECC_DIST_GUARD_H */