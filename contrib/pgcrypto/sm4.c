/* Begin - REQID:SRS-SECRET-PGCRYPTO */
#include "pg_config.h"
#ifdef USE_GMSSL
#include "sm4.h"

static void memxor(void *r, const void *a, size_t len)
{
	uint8_t *pr = r;
	const uint8_t *pa = a;
	size_t i;
	for (i = 0; i < len; i++)
	{
		pr[i] ^= pa[i];
	}

}

static void _memxor(void *r, const void *a, const void *b, size_t len)
{
	uint8_t *pr = r;
	const uint8_t *pa = a;
	const uint8_t *pb = b;
	size_t i;
	for (i = 0; i < len; i++) 
	{
		pr[i] = pa[i] ^ pb[i];
	}
}

void pg_sm4_ecb_encrypt(const SM4_KEY *key,
	const uint8_t *in, size_t nblocks, uint8_t *out)
{
	while (nblocks--) 
	{
		sm4_encrypt(key, in, out);
		in += 16;
		out += 16;
	}
}

void pg_sm4_ecb_decrypt(const SM4_KEY *key,
	const uint8_t *in, size_t nblocks, uint8_t *out)
{
	while (nblocks--) 
	{
		sm4_encrypt(key, in, out);
		in += 16;
		out += 16;
	}
}

void pg_sm4_cbc_encrypt(const SM4_KEY *key, const uint8_t *iv, size_t iv_size,
	const uint8_t *in, size_t nblocks, uint8_t *out)
{
	while (nblocks--) 
	{
		_memxor(out, in, iv, 16);
		sm4_encrypt(key, out, out);
		iv = out;
		in += 16;
		out += 16;
	}
}

void pg_sm4_cbc_decrypt(const SM4_KEY *key, const uint8_t *iv, size_t iv_size,
	const uint8_t *in, size_t nblocks, uint8_t *out)
{
	while (nblocks--) 
	{
		sm4_encrypt(key, in, out);
		memxor(out, iv, 16);
		iv = in;
		in += 16;
		out += 16;
	}
}
#endif
/* End - REQID:SRS-SECRET-PGCRYPTO */
