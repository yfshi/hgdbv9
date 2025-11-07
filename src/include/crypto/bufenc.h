/*-------------------------------------------------------------------------
 *
 * bufenc.h
 *
 * Portions Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
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

#ifdef FRONTEND
extern bool FileEncryptionEnabled;
extern int PG_CIPHER_ENCRYPT_CTR_REL;
extern CryptoKey *LocalRelKey;
#endif

extern void InitializeBufferEncryption(void);
extern void CleanupBufferEncryption(void);
extern void EncryptPage(Page page, bool relation_is_permanent,
						BlockNumber blkno, Size size);
extern void DecryptPage(Page page, bool relation_is_permanent,
						BlockNumber blkno, Size size);

#endif							/* BUFENC_H */
