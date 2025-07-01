/*-------------------------------------------------------------------------
 *
 * kmgr.h
 *
 * Portions Copyright (c) 2025, IvorySQL-2.0
 *
 * src/include/crypto/kmgr.h
 *
 * add the file for requirement "SRS-SECURITY-TDE"
 *-------------------------------------------------------------------------
 */
#ifndef KMGR_H
#define KMGR_H

#include "common/kmgr_utils.h"

#define NUM_KEY_COPIES  3

/* GUC parameters */
extern char *cluster_key_command;

extern Size KmgrShmemSize(void);
extern void KmgrShmemInit(void);
extern void BootStrapKmgr(void);
extern void InitializeKmgr(void);
extern const CryptoKey *KmgrGetKey(int id);

#endif							/* KMGR_H */
