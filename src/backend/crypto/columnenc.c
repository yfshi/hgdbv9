/*
 * Column encryption for supported variable-length attributes.
 *
 * TOAST method 2 is an encryption envelope, not compression. It deliberately
 * expands values, including empty and short strings. The compressed marker
 * ensures every normal detoast consumer authenticates before seeing plaintext.
 * Format: varlena header, tcinfo (plaintext payload size/method), random IV,
 * ciphertext, HMAC-SHA256 over tcinfo + IV + ciphertext. Separate encryption
 * and authentication keys are derived from the cluster relation DEK.
 * Only software CTR and audited datum representations are supported. This protects heap/TOAST storage;
 * expression results and indexes require their own encryption policy.
 */
#include "postgres.h"
#include "access/detoast.h"
#include "access/table.h"
#include "access/htup_details.h"
#include "access/toast_internals.h"
#include "access/xlog.h"
#include "catalog/pg_type_d.h"
#include "common/hmac.h"
#include "crypto/columnenc.h"
#include "crypto/kmgr.h"
#include "utils/rel.h"
#include "utils/memutils.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"

#define COLUMN_IV_SIZE 16
#define COLUMN_TAG_SIZE 32
#define COLUMN_OVERHEAD (sizeof(toast_compress_header) + COLUMN_IV_SIZE + COLUMN_TAG_SIZE)

static void
column_hmac(const unsigned char *key, Size keylen, const unsigned char *data,
			Size len, unsigned char *result)
{
	pg_hmac_ctx *ctx = pg_hmac_create(PG_SHA256);
	if (!ctx || pg_hmac_init(ctx, key, keylen) < 0 ||
		pg_hmac_update(ctx, data, len) < 0 ||
		pg_hmac_final(ctx, result, COLUMN_TAG_SIZE) < 0)
	{
		if (ctx) pg_hmac_free(ctx);
		elog(ERROR, "column encryption HMAC failed");
	}
	pg_hmac_free(ctx);
}

static void
check_column_encryption_enabled(void)
{
	if (!FileEncryptionEnabled || KMGR_USE_HSM_MODE)
		ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						errmsg("column encryption requires software TDE encryption")));
}

/*
 * Physical varlena alone is insufficient: every consumer must detoast before
 * accessing the payload. Keep an explicit list of audited scalar types.
 * Domains retain their normal executor checks and share the base encoding.
 * True arrays use the generic array detoasting path, including arrays of
 * fixed-length elements; this does not enable those elements as scalar columns.
 */
static bool
column_type_supported(Oid typid)
{
	Oid base = getBaseType(typid);

	if (OidIsValid(get_element_type(base)))
		return true;

	switch (base)
	{
		case TEXTOID:
		case VARCHAROID:
		case BPCHAROID:
		case BYTEAOID:
		case NUMERICOID:
		case JSONOID:
		case JSONBOID:
		case XMLOID:
		case BITOID:
		case VARBITOID:
		case INETOID:
		case CIDROID:
		case ORACHARCHAROID:
		case ORACHARBYTEOID:
		case ORAVARCHARCHAROID:
		case ORAVARCHARBYTEOID:
		case ORACHARCHAR_2OID:
		case ORACHARBYTE_2OID:
		case ORAVARCHARCHAR_2OID:
		case ORAVARCHARBYTE_2OID:
		case NUMBEROID:
			return true;
		default:
			return false;
	}
}

static int
column_keys(unsigned char *enc, unsigned char *mac)
{
	const CryptoKey *key;
	const char *enc_label = "PostgreSQL column encryption v1";
	const char *mac_label = "PostgreSQL column authentication v1";
	check_column_encryption_enabled();
	key = KmgrGetKey(KMGR_KEY_ID_REL);
	if (key->klen > COLUMN_TAG_SIZE || key->klen == 0)
		elog(ERROR, "invalid column encryption key length");
	column_hmac(key->key, key->klen, (const unsigned char *) enc_label,
				strlen(enc_label), enc);
	column_hmac(key->key, key->klen, (const unsigned char *) mac_label,
				strlen(mac_label), mac);
	return key->klen;
}

void
CheckColumnEncryption(Oid typid)
{
	if (!column_type_supported(typid))
		ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						errmsg("data type %s is not supported for column encryption",
							   format_type_be(typid)),
						errdetail("Fixed-length scalar types require a different physical storage representation.")));
	check_column_encryption_enabled();
}

bool
ColumnDatumIsEncrypted(struct varlena *value)
{
	if (VARATT_IS_EXTERNAL_ONDISK(value))
	{
		struct varatt_external pointer;
		VARATT_EXTERNAL_GET_POINTER(pointer, value);
		return VARATT_EXTERNAL_GET_COMPRESS_METHOD(pointer) == TOAST_COLUMN_ENCRYPTION_ID;
	}
	return VARATT_IS_COMPRESSED(value) &&
		TOAST_COMPRESS_METHOD(value) == TOAST_COLUMN_ENCRYPTION_ID;
}

bool
RelationHasEncryptedColumns(Relation rel)
{
	int i;
	for (i = 0; i < rel->rd_att->natts; i++)
		if (!TupleDescAttr(rel->rd_att, i)->attisdropped &&
			TupleDescAttr(rel->rd_att, i)->attencrypted)
			return true;
	return false;
}

/* Match the page-encryption policy before adding a column envelope. */
bool
RelationNeedsColumnEncryption(Relation rel)
{
	Relation toastrel;
	bool toast_encrypted;

	if (!FileEncryptionEnabled || AllTableEncrypted)
		return false;
	if (!RelationIsEncrypted(rel))
		return true;
	/* A marked heap alone does not protect out-of-line column values. */
	if (!OidIsValid(rel->rd_rel->reltoastrelid))
		return false;
	toastrel = table_open(rel->rd_rel->reltoastrelid, AccessShareLock);
	toast_encrypted = RelationIsEncrypted(toastrel);
	table_close(toastrel, AccessShareLock);
	return !toast_encrypted;
}

bool
ColumnTupleNeedsTransform(Relation rel, HeapTuple tuple)
{
	int i;
	for (i = 0; i < rel->rd_att->natts; i++)
	{
		Form_pg_attribute att = TupleDescAttr(rel->rd_att, i);
		bool isnull;
		Datum value;
		if (att->attisdropped || att->attlen != -1)
			continue;
		value = heap_getattr(tuple, i + 1, rel->rd_att, &isnull);
		if (!isnull && ((att->attencrypted && RelationNeedsColumnEncryption(rel)) ||
					ColumnDatumIsEncrypted((struct varlena *) DatumGetPointer(value))))
			return true;
	}
	return false;
}

struct varlena *
ColumnEncryptDatum(struct varlena *value)
{
	struct varlena *plain = detoast_attr(value);
	Size len = VARSIZE(plain) - VARHDRSZ;
	struct varlena *result;
	unsigned char enc[32], mac[32];
	unsigned char *iv;
	PgCipherCtx *ctx;
	int outlen, keylen;
	bool ok;
	if (len > MaxAllocSize - COLUMN_OVERHEAD)
		elog(ERROR, "encrypted column value is too large");
	result = palloc(len + COLUMN_OVERHEAD);
	SET_VARSIZE_COMPRESSED(result, len + COLUMN_OVERHEAD);
	TOAST_COMPRESS_SET_SIZE_AND_COMPRESS_METHOD(result, len, TOAST_COLUMN_ENCRYPTION_ID);
	iv = (unsigned char *) result + sizeof(toast_compress_header);
	if (!pg_strong_random(iv, COLUMN_IV_SIZE))
		elog(ERROR, "could not generate column encryption IV");
	keylen = column_keys(enc, mac);
	ctx = pg_cipher_ctx_create(PG_CIPHER_ENCRYPT_CTR, enc, keylen, true);
	if (!ctx || !ctx->ctx)
		elog(ERROR, "could not initialize column encryption");
	ok = pg_cipher_encrypt(ctx, PG_CIPHER_ENCRYPT_CTR,
						(unsigned char *) VARDATA(plain), len,
						iv + COLUMN_IV_SIZE, &outlen, iv, COLUMN_IV_SIZE);
	pg_cipher_ctx_free(ctx);
	if (!ok || outlen != len)
		elog(ERROR, "column encryption failed");
	column_hmac(mac, sizeof(mac), (unsigned char *) result + VARHDRSZ,
				sizeof(uint32) + COLUMN_IV_SIZE + len, iv + COLUMN_IV_SIZE + len);
	explicit_bzero(enc, sizeof(enc));
	explicit_bzero(mac, sizeof(mac));
	if (plain != value) pfree(plain);
	return result;
}

struct varlena *
ColumnDecryptDatum(struct varlena *value)
{
	Size size = VARSIZE(value);
	Size len;
	struct varlena *result;
	unsigned char enc[32], mac[32], tag[32];
	unsigned char *iv;
	unsigned int diff = 0;
	PgCipherCtx *ctx;
	int i, outlen, keylen;
	bool ok;
	if (size < COLUMN_OVERHEAD)
		elog(ERROR, "invalid encrypted column envelope");
	len = TOAST_COMPRESS_EXTSIZE(value);
	if (len != size - COLUMN_OVERHEAD || len > MaxAllocSize - VARHDRSZ)
		elog(ERROR, "invalid encrypted column length");
	iv = (unsigned char *) value + sizeof(toast_compress_header);
	keylen = column_keys(enc, mac);
	column_hmac(mac, sizeof(mac), (unsigned char *) value + VARHDRSZ,
				sizeof(uint32) + COLUMN_IV_SIZE + len, tag);
	for (i = 0; i < COLUMN_TAG_SIZE; i++)
		diff |= tag[i] ^ iv[COLUMN_IV_SIZE + len + i];
	if (diff != 0)
		elog(ERROR, "encrypted column authentication failed");
	result = palloc(len + VARHDRSZ);
	SET_VARSIZE(result, len + VARHDRSZ);
	ctx = pg_cipher_ctx_create(PG_CIPHER_ENCRYPT_CTR, enc, keylen, false);
	if (!ctx || !ctx->ctx)
		elog(ERROR, "could not initialize column decryption");
	ok = pg_cipher_decrypt(ctx, PG_CIPHER_ENCRYPT_CTR, iv + COLUMN_IV_SIZE,
						len, (unsigned char *) VARDATA(result), &outlen,
						iv, COLUMN_IV_SIZE);
	pg_cipher_ctx_free(ctx);
	explicit_bzero(enc, sizeof(enc));
	explicit_bzero(mac, sizeof(mac));
	if (!ok || outlen != len)
		elog(ERROR, "column decryption failed");
	return result;
}
