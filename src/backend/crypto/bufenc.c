/*-------------------------------------------------------------------------
 *
 * bufenc.c
 *
 * Portions Copyright (c) 2025, IvorySQL-2.0
 *
 * IDENTIFICATION
 *	  src/backend/crypto/bufenc.c
 *
 * add the file for requirement "SRS-SECURITY-TDE"
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "miscadmin.h"
#include "lib/stringinfo.h"

#include "access/gist.h"
#include "access/xlog.h"
#include "crypto/bufenc.h"
#include "storage/bufpage.h"
#include "storage/fd.h"

/*
 * We use the page LSN, page number, and permanent-bit to indicate if a fake
 * LSN was used to create a nonce for each page.
 */
#define BUFENC_IV_SIZE		16

static unsigned char buf_encryption_iv[BUFENC_IV_SIZE];

static PgCipherCtx *BufEncCtx = NULL;
static PgCipherCtx *BufDecCtx = NULL;

#ifdef USE_SWSDS
static void* hSessionHandle = NULL;
static void* hKeyHandle = NULL;
#endif

static void set_buffer_encryption_iv(Page page, BlockNumber blkno,
									 bool relation_is_permanent);

void
InitializeBufferEncryption(void)
{
	const CryptoKey *key;

	if (!FileEncryptionEnabled)
		return;

	key = KmgrGetKey(KMGR_KEY_ID_REL);

	if (!KMGR_USE_HSM_MODE)
	{
		BufEncCtx = pg_cipher_ctx_create(PG_CIPHER_ENCRYPT_CTR,
										(unsigned char *) key->key,
										(key->klen), true);
		if (!BufEncCtx || !BufEncCtx->ctx)
			elog(ERROR, "cannot intialize encryption context");

		BufDecCtx = pg_cipher_ctx_create(PG_CIPHER_ENCRYPT_CTR,
										(unsigned char *) key->key,
										(key->klen), false);
		if (!BufDecCtx || !BufDecCtx->ctx)
			elog(ERROR, "cannot intialize decryption context");
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

/* Encrypt the given page with the relation key */
void
EncryptPage(Page page, bool relation_is_permanent, BlockNumber blkno)
{
	unsigned char *ptr = (unsigned char *) page + PageEncryptOffset;
	bool		is_gist_page_or_similar;
	int			enclen;

	/*
	 * Permanent pages have valid LSNs, and non-permanent pages usually have
	 * invalid (not set) LSNs.  (One exception are GiST fake LSNs, see below.)
	 * However, we need valid ones on all pages for encryption.  There are too
	 * many places that set the page LSN for permanent pages to do the same
	 * for non-permanent pages, so we just set it here.
	 *
	 * Also, while permanent relations get new LSNs every time the page is
	 * modified, for non-permanent relations do not, so we just update the LSN
	 * here before it is encrypted.
	 *
	 * GiST indexes uses LSNs, which are also stored in NSN fields, to detect
	 * page splits.  Therefore, we allow the GiST code to assign LSNs and we
	 * don't change them here.
	 */

	/* Permanent relations should already have valid LSNs. */
	Assert(!XLogRecPtrIsInvalid(PageGetLSN(page)) || !relation_is_permanent);

	/*
	 * Check if the page has a special size == GISTPageOpaqueData, a valid
	 * GIST_PAGE_ID, no invalid GiST flag bits are set, and a valid LSN.  This
	 * is true for all GiST pages, and perhaps a few pages that are not.  The
	 * only downside of guessing wrong is that we might not update the LSN for
	 * some non-permanent relation page changes, and therefore reuse the IV,
	 * which seems acceptable.
	 */
	is_gist_page_or_similar =
		(PageGetSpecialSize(page) == MAXALIGN(sizeof(GISTPageOpaqueData)) &&
		 GistPageGetOpaque(page)->gist_page_id == GIST_PAGE_ID &&
		 !XLogRecPtrIsInvalid(PageGetLSN(page)));

	if (!relation_is_permanent && !is_gist_page_or_similar)
		PageSetLSN(page, LSNForEncryption(relation_is_permanent));

	set_buffer_encryption_iv(page, blkno, relation_is_permanent);

	if (!KMGR_USE_HSM_MODE)
	{
		Assert(BufEncCtx != NULL);
		if (unlikely(!pg_cipher_encrypt(BufEncCtx, PG_CIPHER_ENCRYPT_CTR,
										(const unsigned char *) ptr,	/* input  */
										SizeOfPageEncryption,	/* length */
										ptr,	/* output */
										&enclen,	/* resulting length */
										buf_encryption_iv,	/* iv */
										BUFENC_IV_SIZE)))
			elog(ERROR, "cannot encrypt page %u", blkno);
	}
#ifdef USE_SWSDS
	else
	{
		const CryptoKey *key = KmgrGetKey(KMGR_KEY_ID_REL);
		if (unlikely(!HSM_Encrypt(&hSessionHandle, &hKeyHandle,
								  key, hsm_get_kek_id(),
								  HSM_GetAlgID(PG_CIPHER_ENCRYPT_CTR),
								  buf_encryption_iv,
								  ptr, (unsigned int)SizeOfPageEncryption,
								  ptr, (unsigned int*)&enclen)))
			elog(ERROR, "cannot encrypt page %u", blkno);
	}
#endif

	if (unlikely(enclen != SizeOfPageEncryption))
		elog(ERROR, "encrypt error: alg %d in %lu out %d", PG_CIPHER_ENCRYPT_CTR, SizeOfPageEncryption, enclen);
}

/* Decrypt the given page with the relation key */
void
DecryptPage(Page page, bool relation_is_permanent, BlockNumber blkno)
{
	unsigned char *ptr = (unsigned char *) page + PageEncryptOffset;
	int			declen;

	set_buffer_encryption_iv(page, blkno, relation_is_permanent);

	if (!KMGR_USE_HSM_MODE)
	{
		Assert(BufDecCtx != NULL);
		if (unlikely(!pg_cipher_decrypt(BufDecCtx, PG_CIPHER_ENCRYPT_CTR,
										(const unsigned char *) ptr,	/* input  */
										SizeOfPageEncryption,	/* length */
										ptr,	/* output */
										&declen,	/* resulting length */
										buf_encryption_iv,	/* iv */
										BUFENC_IV_SIZE)))
			elog(ERROR, "cannot decrypt page %u", blkno);
	}
#ifdef USE_SWSDS
	else
	{
		const CryptoKey *key = KmgrGetKey(KMGR_KEY_ID_REL);
		if (unlikely(!HSM_Decrypt(&hSessionHandle, &hKeyHandle,
								  key, hsm_get_kek_id(),
								  HSM_GetAlgID(PG_CIPHER_ENCRYPT_CTR),
								  buf_encryption_iv,
								  ptr, (unsigned int)SizeOfPageEncryption,
								  ptr, (unsigned int*)&declen)))
			elog(ERROR, "cannot decrypt page %u", blkno);
	}
#endif

	if (unlikely(declen != SizeOfPageEncryption))
		elog(ERROR, "decrypt error: alg %d in %lu out %d", PG_CIPHER_ENCRYPT_CTR, SizeOfPageEncryption, declen);
}

/* Construct iv for the given page */
static void
set_buffer_encryption_iv(Page page, BlockNumber blkno,
						 bool relation_is_permanent)
{
	unsigned char *p = buf_encryption_iv;

	MemSet(buf_encryption_iv, 0, BUFENC_IV_SIZE);

	/* page lsn (8 byte) */
	memcpy(p, &((PageHeader) page)->pd_lsn, sizeof(PageXLogRecPtr));
	p += sizeof(PageXLogRecPtr);

	/* block number (4 byte) */
	memcpy(p, &blkno, sizeof(BlockNumber));
	p += sizeof(BlockNumber);

	/*
	 * Mark use of fake LSNs in IV so if the real and fake LSN counters
	 * overlap, the IV will remain unique.  XXX Is there a better value?
	 */
	if (!relation_is_permanent)
		*p++ = 0x80;

	/*
	 * The maximum required counter for AES-CTR is 2048, which fits in the
	 * last three bytes.
	 */
}
