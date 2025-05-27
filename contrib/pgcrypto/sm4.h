/* Begin - REQID:SRS-SECRET-PGCRYPTO */
#ifndef SM4_H
#define SM4_H

#include "pg_config.h"
#ifdef USE_GMSSL
#include <gmssl/sm4.h>

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

void pg_sm4_ecb_encrypt(const SM4_KEY *key, const uint8_t *in, size_t nblocks, uint8_t *out);
void pg_sm4_ecb_decrypt(const SM4_KEY *key, const uint8_t *in, size_t nblocks, uint8_t *out);

void pg_sm4_cbc_encrypt(const SM4_KEY *key, const uint8_t *iv, size_t iv_size,
	const uint8_t *in, size_t nblocks, uint8_t *out);
	void pg_sm4_cbc_decrypt(const SM4_KEY *key, const uint8_t *iv, size_t iv_size,
	const uint8_t *in, size_t nblocks, uint8_t *out);

#ifdef __cplusplus
}
#endif

#endif
#endif
/* End - REQID:SRS-SECRET-PGCRYPTO */
