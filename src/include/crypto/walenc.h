/*-------------------------------------------------------------------------
 *
 * walenc.h
 *
 * Portions Copyright (c) 2025, IvorySQL-2.0
 *
 * src/include/crypto/walenc.h
 *
 * add the file for requirement "SRS-SECURITY-TDE"
 *-------------------------------------------------------------------------
 */
#ifndef WALENC_H
#define WALENC_H

#include "storage/bufmgr.h"
#include "crypto/kmgr.h"

#define XLOG_ENCRYPT_PAGES 8
#define XLOG_ENCRYPT_SIZE (XLOG_BLCKSZ * XLOG_ENCRYPT_PAGES)

/*
 * We use the WAL timeline and segment number to create 
 * an IV for each WAL segments.
 */
#define WALENC_IV_SIZE      16
#define WALENC_BLOCK WALENC_IV_SIZE

#ifdef FRONTEND
extern bool WALEncrypted;
extern int PG_CIPHER_ENCRYPT_CTR;
extern CryptoKey *LocalXLogKey;
#endif

extern void InitializeXLogEncryption(void);
extern void EncryptXLog(char *xlog, Size size, TimeLineID timeline, 
						XLogSegNo segment, uint32 counter);
extern void DecryptXLog(char *xlog, Size size, TimeLineID timeline, 
						XLogSegNo segment, uint32 counter);

#endif							/* WALENC_H */
