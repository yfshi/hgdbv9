/*-------------------------------------------------------------------------
 *
 * bufenc.h
 *
 * Portions Copyright (c) 2025, IvorySQL-2.0
 *
 * src/include/crypto/bufenc.h
 *
 * add the file for requirement "SRS-SECURITY-TDE"
 *-------------------------------------------------------------------------
 */
#ifndef BUFENC_H
#define BUFENC_H

#include "storage/bufmgr.h"
#include "crypto/kmgr.h"

/* Cluster encryption encrypts only main forks */
#define PageNeedsToBeEncrypted(forknum) \
	(FileEncryptionEnabled && (forknum) == MAIN_FORKNUM)


extern void InitializeBufferEncryption(void);
extern void EncryptPage(Page page, bool relation_is_permanent,
						BlockNumber blkno);
extern void DecryptPage(Page page, bool relation_is_permanent,
						BlockNumber blkno);

#endif							/* BUFENC_H */
