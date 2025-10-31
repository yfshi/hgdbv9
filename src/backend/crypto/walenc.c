/*-------------------------------------------------------------------------
 *
 * walenc.c
 *
 * Portions Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 *
 * IDENTIFICATION
 *	  src/backend/crypto/walenc.c
 *
 * add the file for requirement "SRS-SECURITY-TDE"
 *-------------------------------------------------------------------------
 */

#ifndef FRONTEND
#include "postgres.h"
#include "utils/memutils.h"
#else
#include "postgres_fe.h"
#endif

#include "access/xlog.h"
#include "access/xlogreader.h"
#include "common/logging.h"
#include "crypto/walenc.h"

static unsigned char wal_encryption_iv[WALENC_IV_SIZE];
static unsigned char wal_iv_bignum[WALENC_IV_SIZE];

static PgCipherCtx *XLogEncCtx = NULL;
static PgCipherCtx *XLogDecCtx = NULL;

#ifdef USE_SWSDS
static void* hSessionHandle = NULL;
static void* hKeyHandle = NULL;
#endif

#ifdef FRONTEND
bool WALEncrypted = false;
int PG_CIPHER_ENCRYPT_CTR_WAL = 0;
CryptoKey *LocalXLogKey = NULL;
static int PG_CIPHER_ENCRYPT_CTR = 0;
#endif

static void EncryptXLog(char *xlog, Size size, TimeLineID timeline, 
						XLogSegNo segment, uint32 counter);
static void DecryptXLog(char *xlog, Size size, TimeLineID timeline, 
						XLogSegNo segment, uint32 counter);
static void set_wal_encryption_iv(TimeLineID timeline,
								  XLogSegNo segment,
								  uint32 counter);

void
InitializeXLogEncryption(void)
{
	const CryptoKey *key;

	if (!WALEncrypted)
		return;

#ifndef FRONTEND
	key = KmgrGetKey(KMGR_KEY_ID_WAL);
#else
	key = LocalXLogKey;
	PG_CIPHER_ENCRYPT_CTR = PG_CIPHER_ENCRYPT_CTR_WAL;
#endif

	if (!KMGR_USE_HSM_MODE)
	{
		XLogEncCtx = pg_cipher_ctx_create(PG_CIPHER_ENCRYPT_CTR,
										(unsigned char *) key->key,
										(key->klen), true);
		if (!XLogEncCtx || !XLogEncCtx->ctx)
#ifndef FRONTEND
			elog(ERROR, "cannot intialize encryption context");
#else
			pg_log_fatal("cannot intialize encryption context");
#endif

		XLogDecCtx = pg_cipher_ctx_create(PG_CIPHER_ENCRYPT_CTR,
										(unsigned char *) key->key,
										(key->klen), false);
		if (!XLogDecCtx || !XLogDecCtx->ctx)
#ifndef FRONTEND
			elog(ERROR, "cannot intialize decryption context");
#else
			pg_log_fatal("cannot intialize decryption context");
#endif
	}
	else
	{
#ifdef USE_SWSDS
		/* create session handle */
		HSM_GetKeyHandle(&hSessionHandle, &hKeyHandle, key, hsm_get_kek_id(), true);
#else
		swsds_failure();
#endif
	}
}

void
CleanupXLogEncryption(void)
{
	if (XLogEncCtx)
		pg_cipher_ctx_free(XLogEncCtx);
	if (XLogDecCtx)
		pg_cipher_ctx_free(XLogDecCtx);
}

void
XLogEncryptInplace(char *xlog, Size size, TimeLineID timeline,
				   XLogSegNo segment, uint32 offset)
{
	uint32		counter;
	uint32		shift;
	uint32		partlen;

	if (!WALEncrypted)
		return;

	/* Calculate the counter */
	counter = offset / WALENC_BLOCK;
	shift = offset % WALENC_BLOCK;

	/* There have an uncompleted block */
	if (shift > 0)
	{
		unsigned char xlogPart[WALENC_BLOCK];
		
		partlen = Min(WALENC_BLOCK - shift, size);
		memcpy(xlogPart + shift, xlog, partlen);
		EncryptXLog((char *) xlogPart, (Size) WALENC_BLOCK,
					timeline, segment, counter);
		memcpy(xlog, xlogPart + shift, partlen);

		xlog += partlen;
		size -= partlen;
		counter++;
	}

	if (size > 0)
		EncryptXLog(xlog, size, timeline, segment, counter);
}

void
XLogDecryptInplace(char *xlog, Size size, TimeLineID timeline,
				   XLogSegNo segment, uint32 offset)
{
	uint32		counter;
	uint32		shift;
	uint32		partlen;

	if (!WALEncrypted)
		return;

	/* Calculate the counter */
	counter = offset / WALENC_BLOCK;
	shift = offset % WALENC_BLOCK;

	/* There have an uncompleted block */
	if (shift > 0)
	{
		unsigned char xlogPart[WALENC_BLOCK];

		partlen = Min(WALENC_BLOCK - shift, size);
		memcpy(xlogPart + shift, xlog, partlen);
		DecryptXLog((char *) xlogPart, (Size) WALENC_BLOCK,
					timeline, segment, counter);
		memcpy(xlog, xlogPart + shift, partlen);

		xlog += partlen;
		size -= partlen;
		counter++;
	}

	if (size > 0)
		DecryptXLog(xlog, size, timeline, segment, counter);
}

/* Encrypt the given wal segment with the wal key */
static void
EncryptXLog(char *xlog, Size size, TimeLineID timeline, 
			XLogSegNo segment, uint32 counter)
{
	unsigned char *ptr = (unsigned char *) xlog;
	int		enclen;

	if (size <= 0)
		return;

	set_wal_encryption_iv(timeline, segment, counter);

	if (!KMGR_USE_HSM_MODE)
	{
		Assert(XLogEncCtx != NULL);
		if (unlikely(!pg_cipher_encrypt(XLogEncCtx, PG_CIPHER_ENCRYPT_CTR,
										ptr,	/* input  */
										size,	/* length */
										ptr,	/* output */
										&enclen,	/* resulting length */
										wal_encryption_iv,	/* iv */
										WALENC_IV_SIZE)))
#ifndef FRONTEND
			elog(ERROR, "cannot encrypt wal segment number %lu", segment);
#else
			pg_log_fatal("cannot encrypt wal segment number %lu", segment);
#endif
	}
#ifdef USE_SWSDS
	else
	{
		const CryptoKey *key;

#ifndef FRONTEND
		key = KmgrGetKey(KMGR_KEY_ID_WAL);
#else
		key = LocalXLogKey;
#endif
		if (unlikely(!HSM_Encrypt(&hSessionHandle, &hKeyHandle,
								  key, hsm_get_kek_id(),
								  HSM_GetAlgID(PG_CIPHER_ENCRYPT_CTR),
								  wal_encryption_iv,
								  ptr, (unsigned int)size,
								  ptr, (unsigned int*)&enclen)))
#ifndef FRONTEND
			elog(ERROR, "cannot encrypt wal segment number %lu", segment);
#else
			pg_log_fatal("cannot encrypt wal segment number %lu", segment);
#endif
	}
#endif

	Assert(enclen == size);
}

/* Decrypt the given page with the relation key */
static void
DecryptXLog(char *xlog, Size size, TimeLineID timeline, 
			XLogSegNo segment, uint32 counter)
{
	unsigned char *ptr = (unsigned char *) xlog;
	int		declen;

	if (size <= 0)
		return;

	set_wal_encryption_iv(timeline, segment, counter);
	if (!KMGR_USE_HSM_MODE)
	{
		Assert(XLogDecCtx != NULL);

		if (unlikely(!pg_cipher_decrypt(XLogDecCtx, PG_CIPHER_ENCRYPT_CTR,
										ptr,	/* input  */
										size,	/* length */
										ptr,	/* output */
										&declen,	/* resulting length */
										wal_encryption_iv,	/* iv */
										WALENC_IV_SIZE)))
#ifndef FRONTEND
			elog(ERROR, "cannot decrypt wal segment number %lu", segment);
#else
			pg_log_fatal("cannot decrypt wal segment number %lu", segment);
#endif
	}
#ifdef USE_SWSDS
	else
	{
		const CryptoKey *key;

#ifndef FRONTEND
		key = KmgrGetKey(KMGR_KEY_ID_WAL);
#else
		key = LocalXLogKey;
#endif
		if (unlikely(!HSM_Decrypt(&hSessionHandle, &hKeyHandle,
								  key, hsm_get_kek_id(),
								  HSM_GetAlgID(PG_CIPHER_ENCRYPT_CTR),
								  wal_encryption_iv,
								  ptr, (unsigned int)size,
								  ptr, (unsigned int*)&declen)))
#ifndef FRONTEND
			elog(ERROR, "cannot decrypt wal segment number %lu", segment);
#else
			pg_log_fatal("cannot decrypt wal segment number %lu", segment);
#endif
	}
#endif

	Assert(declen == size);
}

/* Construct iv for the given page */
static void
set_wal_encryption_iv(TimeLineID timeline, XLogSegNo segment, uint32 counter)
{
	unsigned char  *p = wal_encryption_iv;
	int				index;
	int				temp;

	MemSet(wal_encryption_iv, 0, WALENC_IV_SIZE);

	/* wal timeline (4 byte) */
	memcpy(p, &timeline, sizeof(TimeLineID));
	p += sizeof(TimeLineID);

	/* wal segment nuber (8 byte) */
	memcpy(p, &segment, sizeof(XLogSegNo));

	/* set the counter */
	for (index = WALENC_IV_SIZE - 1; index >= 0; index--)
	{
		temp = (int) wal_encryption_iv[index] + counter;
		wal_iv_bignum[index] = temp % 256;
		counter = temp / 256;
	}
	index = 0;

	/* carry */
	if (counter)
	{
		StringInfoData extra;
		int	extralen = 0;

		initStringInfo(&extra);
		while (counter)
		{
			appendStringInfoChar(&extra, (char) (counter % 256));
			counter = counter / 256;
			extralen++;
		}
		while (extralen > 0 && index < WALENC_IV_SIZE)
			wal_encryption_iv[index++] = extra.data[--extralen];

		pfree(extra.data);
	}

	if (index < WALENC_IV_SIZE)
		memcpy(wal_encryption_iv + index, wal_iv_bignum, WALENC_IV_SIZE - index);
}
